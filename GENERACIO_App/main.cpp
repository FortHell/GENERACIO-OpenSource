#include <QtWidgets>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QTimer>
#include <QSettings>
#include <QPainter>
#include <QPropertyAnimation>
#include <QDesktopServices>
#include <QProcess>
#include <QElapsedTimer>
#include "HMDTile.h"

// --- Scan Line Overlay ---
class ScanLineOverlay : public QWidget {
    Q_OBJECT
        Q_PROPERTY(float progress READ progress WRITE setProgress)
public:
    ScanLineOverlay(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);

        anim = new QPropertyAnimation(this, "progress", this);
        anim->setDuration(2200);
        anim->setStartValue(0.0f);
        anim->setEndValue(1.0f);
        anim->setEasingCurve(QEasingCurve::Linear);

        connect(anim, &QPropertyAnimation::finished, this, &ScanLineOverlay::scheduleNext);
        scheduleNext();
    }

    float progress() const { return m_progress; }
    void setProgress(float p) {
        m_progress = p;
        if (p < 0.15f)
            m_opacity = p / 0.15f;
        else if (p > 0.85f)
            m_opacity = (1.0f - p) / 0.15f;
        else
            m_opacity = 1.0f;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (m_progress <= 0.0f || m_progress >= 1.0f) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        float x = m_progress * (width() + 300) - 150;

        QLinearGradient grad(x - 120, 0, x + 120, 0);
        grad.setColorAt(0.0, QColor(0, 0, 0, 0));
        grad.setColorAt(0.3, QColor(100, 180, 255, int(18 * m_opacity)));
        grad.setColorAt(0.5, QColor(160, 220, 255, int(55 * m_opacity)));
        grad.setColorAt(0.7, QColor(100, 180, 255, int(18 * m_opacity)));
        grad.setColorAt(1.0, QColor(0, 0, 0, 0));

        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRect(QRectF(x - 120, m_lineY - 6, 240, 12));

        QLinearGradient core(x - 40, 0, x + 40, 0);
        core.setColorAt(0.0, QColor(0, 0, 0, 0));
        core.setColorAt(0.5, QColor(200, 235, 255, int(120 * m_opacity)));
        core.setColorAt(1.0, QColor(0, 0, 0, 0));

        p.setBrush(core);
        p.drawRect(QRectF(x - 40, m_lineY - 1, 80, 2));
    }

private slots:
    void scheduleNext() {
        float margin = height() * 0.075f;
        m_lineY = margin + QRandomGenerator::global()->bounded((int)(height() * 0.85f));
        int delay = QRandomGenerator::global()->bounded(500, 2000);
        QTimer::singleShot(delay, this, [this]() { anim->start(); });
    }

private:
    QPropertyAnimation* anim;
    float m_progress = 0.0f;
    float m_opacity = 0.0f;
    float m_lineY = 0.0f;
};

// --- Helpers ---
QPushButton* createBackButton(QWidget* parent = nullptr) {
    QPushButton* backBtn = new QPushButton("<", parent);
    backBtn->setFixedSize(48, 48);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 32px;
            color: #888;
            background-color: transparent;
            border: none;
            font-weight: bold;
        }
        QPushButton:hover { color: #00aaff; }
    )");
    return backBtn;
}

QPushButton* createNavButton(const QString& label, QWidget* parent = nullptr) {
    QPushButton* btn = new QPushButton(label, parent);
    btn->setFixedHeight(48);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            color: #888;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: bold;
            text-align: left;
            padding-left: 16px;
        }
        QPushButton:hover { background-color: #2a2a2a; color: #ccc; }
        QPushButton:checked { background-color: #1e3a4f; color: #00aaff; }
    )");
    return btn;
}

// --- Device info row ---
QWidget* createDeviceRow(const QString& label, QWidget* valueWidget) {
    QWidget* row = new QWidget();
    row->setFixedHeight(52);
    row->setStyleSheet("background-color: #222222; border-radius: 6px;");

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(20, 0, 20, 0);

    QLabel* lbl = new QLabel(label);
    lbl->setStyleSheet("color: #666; font-size: 11pt;");
    layout->addWidget(lbl);
    layout->addStretch();
    layout->addWidget(valueWidget);
    return row;
}

QWidget* createDeviceRow(const QString& label, const QString& value) {
    QLabel* val = new QLabel(value);
    val->setStyleSheet("color: #aaa; font-size: 11pt;");
    return createDeviceRow(label, val);
}

class AppWindow : public QMainWindow {
    Q_OBJECT
public:
    AppWindow() {
        setWindowTitle("GENERACIO App");
        setWindowFlags(Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        QWidget* mainWidget = new QWidget();
        mainWidget->setStyleSheet("background-color: #1a1a1a; border-radius: 8px;");
        QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Title Bar
        QWidget* titleBar = new QWidget();
        titleBar->setFixedHeight(45);
        titleBar->setStyleSheet("background-color: #252525; border-top-left-radius: 8px; border-top-right-radius: 8px;");
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(15, 0, 5, 0);

        QLabel* windowTitle = new QLabel("GENERACIO Home");
        windowTitle->setStyleSheet("color: #6200b3; font-size: 22px; font-weight: bold; letter-spacing: 3px;");
        titleLayout->addWidget(windowTitle);
        titleLayout->addStretch();

        auto addTitleBtn = [&](const QString& iconPath, auto slot) {
            QPushButton* btn = new QPushButton();
            btn->setIcon(QIcon(iconPath));
            btn->setIconSize(QSize(16, 16));
            btn->setFixedSize(38, 38);
            btn->setStyleSheet(R"(
                QPushButton { background: transparent; border: none; }
                QPushButton:hover { background-color: #444444; border-radius: 4px; }
                QPushButton:pressed { background-color: #222222; }
            )");
            connect(btn, &QPushButton::clicked, this, slot);
            titleLayout->addWidget(btn);
            };

        addTitleBtn("ui/window_minimize.png", &QMainWindow::showMinimized);
        addTitleBtn("ui/window_maximize.png", [=]() { isMaximized() ? showNormal() : showMaximized(); });
        addTitleBtn("ui/window_close.png", &QMainWindow::close);

        mainLayout->addWidget(titleBar);

        stacked = new QStackedWidget();
        mainLayout->addWidget(stacked);
        setCentralWidget(mainWidget);

        setupSelectionPage();
        setupSettingsPage();
        setupMainScreen();

        QScreen* screen = QGuiApplication::primaryScreen();
        resize(screen->availableGeometry().size() * 0.7);

        if (isSetupComplete()) {
            stacked->setCurrentWidget(mainScreen);
        }
    }

private:
    QString configPath() {
        return QCoreApplication::applicationDirPath() + "/generacio.ini";
    }
    bool isSetupComplete() {
        QSettings cfg(configPath(), QSettings::IniFormat);
        return cfg.value("setup/complete", false).toBool();
    }
    void markSetupComplete() {
        QSettings cfg(configPath(), QSettings::IniFormat);
        cfg.setValue("setup/complete", true);
        cfg.sync();
    }
    void resetSetup() {
        QSettings cfg(configPath(), QSettings::IniFormat);
        cfg.setValue("setup/complete", false);
        cfg.sync();
    }
    void saveDeviceName(const QString& name) {
        QSettings cfg(configPath(), QSettings::IniFormat);
        cfg.setValue("device/name", name);
        cfg.sync();
    }
    QString loadDeviceName() {
        QSettings cfg(configPath(), QSettings::IniFormat);
        return cfg.value("device/name", "Unknown").toString();
    }
    void saveDeviceSetting(const QString& key, const QString& value) {
        QSettings cfg(configPath(), QSettings::IniFormat);
        cfg.setValue("device/" + key, value);
        cfg.sync();
    }
    QString loadDeviceSetting(const QString& key, const QString& def) {
        QSettings cfg(configPath(), QSettings::IniFormat);
        return cfg.value("device/" + key, def).toString();
    }

    void setupSelectionPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 40, 20, 40);

        QLabel* title = new QLabel("Select your Device");
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("color: white; font-size: 28pt; font-weight: bold;");
        layout->addStretch(1);
        layout->addWidget(title);
        layout->addSpacing(30);

        QHBoxLayout* tilesLayout = new QHBoxLayout();
        tilesLayout->setSpacing(40);

        HMDTile* t1 = new HMDTile("Reality 1");
        HMDTile* t2 = new HMDTile("Reality 1+");

        auto setupFluidTile = [](HMDTile* tile) {
            tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            tile->setMinimumSize(250, 350);
            tile->setMaximumSize(450, 600);
            };

        setupFluidTile(t1);
        setupFluidTile(t2);

        connect(t1, &HMDTile::clicked, this, &AppWindow::onDeviceSelected);
        connect(t2, &HMDTile::clicked, this, &AppWindow::onDeviceSelected);

        tilesLayout->addStretch(2);
        tilesLayout->addWidget(t1);
        tilesLayout->addWidget(t2);
        tilesLayout->addStretch(2);

        layout->addLayout(tilesLayout);
        layout->addStretch(2);
        stacked->addWidget(page);
    }

    void setupSettingsPage() {
        settingsPage = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(settingsPage);
        layout->setContentsMargins(30, 20, 30, 30);

        QHBoxLayout* header = new QHBoxLayout();
        QPushButton* backBtn = createBackButton();
        connect(backBtn, &QPushButton::clicked, this, [=]() {
            stopCameraCheck();
            stacked->setCurrentIndex(0);
            });
        header->addWidget(backBtn);
        header->addStretch();
        layout->addLayout(header);

        layout->addStretch(1);

        deviceTitleLabel = new QLabel();
        deviceTitleLabel->setAlignment(Qt::AlignCenter);
        deviceTitleLabel->setStyleSheet("color: white; font-size: 24pt; font-weight: bold;");
        layout->addWidget(deviceTitleLabel);

        statusText = new QLabel();
        statusText->setAlignment(Qt::AlignCenter);
        statusText->setWordWrap(true);
        statusText->setStyleSheet("color: #bbb; font-size: 14pt; margin-top: 20px;");
        layout->addWidget(statusText);

        layout->addStretch(2);

        nextBtn = new QPushButton("Next");
        nextBtn->setFixedSize(160, 55);
        nextBtn->setCursor(Qt::PointingHandCursor);
        nextBtn->setEnabled(false);
        nextBtn->setStyleSheet(
            "QPushButton { background-color: #333; color: #666; border-radius: 6px; font-weight: bold; font-size: 12pt; }"
            "QPushButton:enabled { background-color: #00aaff; color: white; }"
        );
        connect(nextBtn, &QPushButton::clicked, this, [=]() {
            stopCameraCheck();
            markSetupComplete();
            saveDeviceName(selectedName);
            stacked->setCurrentWidget(mainScreen);
            });

        devskipBtn = new QPushButton("DEV: Skip");
        devskipBtn->setFixedSize(160, 55);
        devskipBtn->setCursor(Qt::PointingHandCursor);
        devskipBtn->setStyleSheet(
            "QPushButton { background-color: #381717; color: #ffffff; border-radius: 6px; font-weight: bold; font-size: 12pt; }"
        );
        connect(devskipBtn, &QPushButton::clicked, this, [=]() {
            stopCameraCheck();
            markSetupComplete();
            saveDeviceName(selectedName);
            stacked->setCurrentWidget(mainScreen);
            });

        QHBoxLayout* footer = new QHBoxLayout();
        footer->addStretch();
        footer->addWidget(devskipBtn);
        footer->addWidget(nextBtn);
        layout->addLayout(footer);

        stacked->addWidget(settingsPage);
    }

    QWidget* buildDevicesPage() {
        QString deviceName = loadDeviceName();
        QString resolution = loadDeviceSetting("resolution", "2160x2160");
        QString trackingHz = loadDeviceSetting("tracking_hz", "90Hz");

        QWidget* page = new QWidget();
        page->setStyleSheet("background-color: #1a1a1a;");
        QVBoxLayout* outer = new QVBoxLayout(page);
        outer->setContentsMargins(40, 40, 40, 40);
        outer->setSpacing(0);

        // --- Header ---
        QHBoxLayout* headerLayout = new QHBoxLayout();

        // Status dot
        QLabel* dot = new QLabel();
        dot->setFixedSize(14, 14);
        dot->setObjectName("deviceDot");
        dot->setStyleSheet("background-color: #cc3333; border-radius: 7px;");

        QLabel* nameLbl = new QLabel(deviceName);
        nameLbl->setStyleSheet("color: white; font-size: 22pt; font-weight: bold;");

        statusLbl = new QLabel("Disconnected");
        statusLbl->setObjectName("deviceStatus");
        statusLbl->setStyleSheet("color: #cc3333; font-size: 11pt; font-weight: bold; margin-left: 8px;");

        headerLayout->addWidget(nameLbl);
        headerLayout->addSpacing(12);
        headerLayout->addWidget(dot);
        headerLayout->addWidget(statusLbl);
        headerLayout->addStretch();
        outer->addLayout(headerLayout);
        outer->addSpacing(8);

        // Thin divider
        QFrame* divider = new QFrame();
        divider->setFrameShape(QFrame::HLine);
        divider->setStyleSheet("color: #2a2a2a;");
        outer->addWidget(divider);
        outer->addSpacing(24);

        // --- Rows ---
        QVBoxLayout* rows = new QVBoxLayout();
        rows->setSpacing(8);

        // Firmware
        rows->addWidget(createDeviceRow("Firmware version", "v0.1-beta"));

        // USB version
        rows->addWidget(createDeviceRow("USB version", "—"));

        // Per-eye resolution (clickable)
        QComboBox* resBox = new QComboBox();
        resBox->addItems({ { "Performance  2160x2160", "Standard  2592x2592", "High  3024x3024", "Ultra  3300x3300"} });
        resBox->setCurrentText(resolution);
        resBox->setStyleSheet(R"(
            QComboBox {
                background-color: #2a2a2a;
                color: #aaa;
                border: 1px solid #3a3a3a;
                border-radius: 4px;
                padding: 4px 10px;
                font-size: 11pt;
                min-width: 140px;
            }
            QComboBox::drop-down { border: none; }
            QComboBox QAbstractItemView {
                background-color: #2a2a2a;
                color: #aaa;
                selection-background-color: #1e3a4f;
            }
        )");
        connect(resBox, &QComboBox::currentTextChanged, this, [=](const QString& val) {
            saveDeviceSetting("resolution", val);
            });
        rows->addWidget(createDeviceRow("Per-eye resolution", resBox));

        // Tracking frequency (clickable)
        QComboBox* hzBox = new QComboBox();
        hzBox->addItems({ "60Hz", "90Hz", "120Hz" });
        hzBox->setCurrentText(trackingHz);
        hzBox->setStyleSheet(resBox->styleSheet());
        connect(hzBox, &QComboBox::currentTextChanged, this, [=](const QString& val) {
            saveDeviceSetting("tracking_hz", val);
            });
        rows->addWidget(createDeviceRow("Tracking frequency", hzBox));

        outer->addLayout(rows);
        outer->addStretch();

        // --- Camera check timer (updates dot + status label) ---
        deviceDot = dot;
        QTimer* camTimer = new QTimer(page);
        camTimer->setInterval(1000);
        connect(camTimer, &QTimer::timeout, this, [=]() {
            const auto cameras = QMediaDevices::videoInputs();
            bool found = false;
            for (const auto& cam : cameras) {
                QString id = QString::fromLatin1(cam.id()).toLower();
                if (id.contains("1e45") || id.contains("0209")) { found = true; break; }
            }
            QString dotStyle = found
                ? "background-color: #00cc66; border-radius: 7px;"
                : "background-color: #cc3333; border-radius: 7px;";
            QString statusStyle = found
                ? "color: #00cc66; font-size: 11pt; font-weight: bold; margin-left: 8px;"
                : "color: #cc3333; font-size: 11pt; font-weight: bold; margin-left: 8px;";
            dot->setStyleSheet(dotStyle);
            statusLbl->setStyleSheet(statusStyle);
            statusLbl->setText(found ? "Connected" : "Disconnected");
            QString deviceName = loadDeviceName();
            nameLbl->setText(deviceName);
            });
        camTimer->start();

        return page;
    }

    void setupMainScreen() {
        mainScreen = new QWidget();
        QHBoxLayout* rootLayout = new QHBoxLayout(mainScreen);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        // --- Left Nav Panel ---
        QWidget* navPanel = new QWidget();
        navPanel->setFixedWidth(200);
        navPanel->setStyleSheet("background-color: #202020; border-right: 1px solid #2e2e2e;");
        QVBoxLayout* navLayout = new QVBoxLayout(navPanel);
        navLayout->setContentsMargins(12, 24, 12, 24);
        navLayout->setSpacing(6);

        QPushButton* btnHome = createNavButton("Home");
        QPushButton* btnDevices = createNavButton("Devices");
        QPushButton* btnSettings = createNavButton("Settings");

        navLayout->addWidget(btnHome);
        navLayout->addWidget(btnDevices);
        navLayout->addWidget(btnSettings);
        navLayout->addStretch();

        QFrame* sep = new QFrame();
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #2e2e2e;");
        navLayout->addWidget(sep);

        QPushButton* btnBackToStart = createNavButton("BackToStart");
        btnBackToStart->setCheckable(false);
        btnBackToStart->setStyleSheet(btnBackToStart->styleSheet() +
            "QPushButton { color: #664444; }"
            "QPushButton:hover { color: #ff6666; background-color: #2a1a1a; }"
        );
        navLayout->addWidget(btnBackToStart);
        rootLayout->addWidget(navPanel);

        // --- Content Stack ---
        contentStack = new QStackedWidget();
        contentStack->setStyleSheet("background-color: #1a1a1a;");
        rootLayout->addWidget(contentStack);

        // Home page
        QWidget* homePage = new QWidget();
        homePage->setStyleSheet("background-color: #1a1a1a;");
        QVBoxLayout* homeLayout = new QVBoxLayout(homePage);
        homeLayout->setAlignment(Qt::AlignCenter);

        QLabel* welcomeTitle = new QLabel("Welcome back.");
        welcomeTitle->setAlignment(Qt::AlignCenter);
        welcomeTitle->setStyleSheet("color: white; font-size: 28pt; font-weight: bold;");
        QLabel* welcomeSub = new QLabel("Your device is ready.");
        welcomeSub->setAlignment(Qt::AlignCenter);
        welcomeSub->setStyleSheet("color: #555; font-size: 14pt; margin-top: 12px;");
        homeLayout->addWidget(welcomeTitle);
        homeLayout->addWidget(welcomeSub);

        QPushButton* launchBtn = new QPushButton("Launch SteamVR");
        launchBtn->setFixedSize(220, 55);
        launchBtn->setCursor(Qt::PointingHandCursor);
        launchBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #6200b3;
                color: white;
                border-radius: 8px;
                font-size: 13pt;
                font-weight: bold;
            }
            QPushButton:hover { background-color: #7a00e6; }
            QPushButton:pressed { background-color: #4a0088; }
            QPushButton:disabled { background-color: #2a2a2a; color: #555; }
        )");

        QLabel* uptimeLabel = new QLabel("");
        uptimeLabel->setAlignment(Qt::AlignCenter);
        uptimeLabel->setStyleSheet("color: #444; font-size: 11pt; font-family: monospace;");

        homeLayout->addSpacing(24);
        homeLayout->addWidget(launchBtn, 0, Qt::AlignCenter);
        homeLayout->addSpacing(8);
        homeLayout->addWidget(uptimeLabel, 0, Qt::AlignCenter);

        QTimer* vrCheckTimer = new QTimer(this);
        vrCheckTimer->setInterval(250);
        QElapsedTimer* elapsed = new QElapsedTimer();
        QTimer* uptimeTicker = new QTimer(this);
        uptimeTicker->setInterval(1000);

        connect(uptimeTicker, &QTimer::timeout, this, [uptimeLabel, elapsed]() {
            qint64 secs = elapsed->elapsed() / 1000;
            int h = secs / 3600;
            int m = (secs % 3600) / 60;
            int s = secs % 60;
            uptimeLabel->setText(QString("Elapsed:  %1:%2:%3")
                .arg(h, 2, 10, QChar('0'))
                .arg(m, 2, 10, QChar('0'))
                .arg(s, 2, 10, QChar('0')));
            });

        connect(vrCheckTimer, &QTimer::timeout, this, [=]() {
            QProcess proc;
            proc.start("tasklist", { "/FI", "IMAGENAME eq vrserver.exe", "/NH" });
            proc.waitForFinished(200);
            bool running = proc.readAllStandardOutput().contains("vrserver.exe");

            if (running && !uptimeTicker->isActive()) {
                elapsed->start();
                uptimeTicker->start();
                vrCheckTimer->setInterval(1000);
                launchBtn->setText("SteamVR Running");
                launchBtn->setEnabled(false);
                uptimeLabel->setStyleSheet("color: #00aa66; font-size: 11pt; font-family: monospace;");
            }
            else if (!running && uptimeTicker->isActive()) {
                uptimeTicker->stop();
                uptimeLabel->setText("");
                uptimeLabel->setStyleSheet("color: #444; font-size: 11pt; font-family: monospace;");
                launchBtn->setText("Launch SteamVR");
                launchBtn->setEnabled(true);
                vrCheckTimer->setInterval(250);
            }
            });

        connect(launchBtn, &QPushButton::clicked, this, [=]() {
            launchBtn->setText("Launching...");
            launchBtn->setEnabled(false);
            uptimeLabel->setStyleSheet("color: #555; font-size: 11pt; font-family: monospace;");
            uptimeLabel->setText("waiting for SteamVR...");
            QDesktopServices::openUrl(QUrl("steam://run/250820"));
            vrCheckTimer->start();
            });

        ScanLineOverlay* scanLine = new ScanLineOverlay(homePage);
        scanLine->setAttribute(Qt::WA_TransparentForMouseEvents);

        class ResizeFilter : public QObject {
        public:
            ScanLineOverlay* overlay;
            ResizeFilter(QObject* parent, ScanLineOverlay* o) : QObject(parent), overlay(o) {}
            bool eventFilter(QObject* obj, QEvent* e) override {
                if (e->type() == QEvent::Resize) {
                    auto* w = static_cast<QWidget*>(obj);
                    overlay->setGeometry(w->rect());
                }
                return false;
            }
        };
        homePage->installEventFilter(new ResizeFilter(homePage, scanLine));
        contentStack->addWidget(homePage);

        // Devices page
        contentStack->addWidget(buildDevicesPage());

        // Settings placeholder
        QWidget* settingsPageMain = new QWidget();
        QVBoxLayout* setLayout = new QVBoxLayout(settingsPageMain);
        setLayout->setAlignment(Qt::AlignCenter);
        QLabel* setLabel = new QLabel("Settings");
        setLabel->setAlignment(Qt::AlignCenter);
        setLabel->setStyleSheet("color: #555; font-size: 20pt; font-weight: bold;");
        setLayout->addWidget(setLabel);
        contentStack->addWidget(settingsPageMain);

        // Nav logic
        auto selectNav = [=](QPushButton* active, int index) {
            for (auto* b : { btnHome, btnDevices, btnSettings })
                b->setChecked(false);
            active->setChecked(true);
            contentStack->setCurrentIndex(index);
            };

        connect(btnHome, &QPushButton::clicked, this, [=]() { selectNav(btnHome, 0); });
        connect(btnDevices, &QPushButton::clicked, this, [=]() { selectNav(btnDevices, 1); });
        connect(btnSettings, &QPushButton::clicked, this, [=]() { selectNav(btnSettings, 2); });
        connect(btnBackToStart, &QPushButton::clicked, this, [=]() {
            resetSetup();
            stacked->setCurrentIndex(0);
            });

        btnHome->setChecked(true);
        stacked->addWidget(mainScreen);
    }

private slots:
    void onDeviceSelected(const QString& name) {
        selectedName = name;
        deviceTitleLabel->setText(name + " Setup");
        statusText->setText("Step 1: Please connect the USB Camera to your PC.");
        nextBtn->setEnabled(false);
        stacked->setCurrentWidget(settingsPage);
        startCameraCheck();
    }

    void checkCamera() {
        if (cameraFound) return;
        const auto cameras = QMediaDevices::videoInputs();
        for (const auto& cam : cameras) {
            QString id = QString::fromLatin1(cam.id()).toLower();
            if (id.contains("1e45") || id.contains("0209")) {
                cameraFound = true;
                if (cameraCheckTimer) cameraCheckTimer->stop();
                statusText->setText("Hardware Detected! Click 'Next' to continue.");
                statusText->setStyleSheet("color: #00ff88; font-size: 14pt; font-weight: bold;");
                nextBtn->setEnabled(true);
                break;
            }
        }
    }

    void startCameraCheck() {
        cameraFound = false;
        statusText->setStyleSheet("color: #bbb; font-size: 14pt;");
        if (!cameraCheckTimer) {
            cameraCheckTimer = new QTimer(this);
            cameraCheckTimer->setInterval(1000);
            connect(cameraCheckTimer, &QTimer::timeout, this, &AppWindow::checkCamera);
        }
        cameraCheckTimer->start();
    }

    void stopCameraCheck() {
        if (cameraCheckTimer) cameraCheckTimer->stop();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && event->pos().y() < 45) {
            dragging = true;
            dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging) move(event->globalPosition().toPoint() - dragStart);
    }
    void mouseReleaseEvent(QMouseEvent*) override { dragging = false; }

private:
    QStackedWidget* stacked;
    QWidget* settingsPage;
    QWidget* mainScreen;
    QStackedWidget* contentStack;
    QLabel* deviceTitleLabel;
    QLabel* statusText;
    QLabel* statusLbl = nullptr;
    QLabel* deviceDot = nullptr;
    QPushButton* nextBtn;
    QPushButton* devskipBtn;
    QTimer* cameraCheckTimer = nullptr;
    bool cameraFound = false;
    bool dragging = false;
    QPoint dragStart;
    QString selectedName;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    AppWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"