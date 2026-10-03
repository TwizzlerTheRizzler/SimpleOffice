#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QStyleFactory>
#include <QPalette>
#include <QMessageBox>

class SimpleOfficeLauncher : public QMainWindow {
    Q_OBJECT

public:
    SimpleOfficeLauncher(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("SimpleOffice");
        setFixedSize(500, 320);

        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(30, 25, 30, 25);
        mainLayout->setSpacing(20);

        // Header
        QLabel *titleLabel = new QLabel("SimpleOffice", this);
        titleLabel->setStyleSheet("font-size: 26px; font-weight: bold; color: #111111;");
        titleLabel->setAlignment(Qt::AlignCenter);

        QLabel *subTitleLabel = new QLabel("Select an application to start", this);
        subTitleLabel->setStyleSheet("font-size: 13px; color: #666666;");
        subTitleLabel->setAlignment(Qt::AlignCenter);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(subTitleLabel);
        mainLayout->addSpacing(10);

        // App Buttons Layout
        QHBoxLayout *appsLayout = new QHBoxLayout();
        appsLayout->setSpacing(15);

        QPushButton *writeBtn = createCardButton("SimpleWrite", "Text Editor");
        QPushButton *sheetBtn = createCardButton("SimpleSheet", "Spreadsheet");

        appsLayout->addWidget(writeBtn);
        appsLayout->addWidget(sheetBtn);
        mainLayout->addLayout(appsLayout);

        // Footer version string
        

        setCentralWidget(centralWidget);

        connect(writeBtn, &QPushButton::clicked, this, [this]() { launchApp("SimpleWrite.exe"); });
        connect(sheetBtn, &QPushButton::clicked, this, [this]() { launchApp("SimpleSheet.exe"); });
    }

private:
    QPushButton* createCardButton(const QString &title, const QString &subtitle) {
        QPushButton *btn = new QPushButton(this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedHeight(110);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #ffffff;"
            "  border: 2px solid #e0e0e0;"
            "  border-radius: 8px;"
            "  font-size: 16px;"
            "  font-weight: bold;"
            "  color: #222222;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f0f7ff;"
            "  border-color: #2a82da;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #e3f2fd;"
            "}"
        );

        QVBoxLayout *layout = new QVBoxLayout(btn);
        QLabel *tLabel = new QLabel(title, btn);
        tLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #222222; border: none; background: transparent;");
        tLabel->setAlignment(Qt::AlignCenter);

        QLabel *sLabel = new QLabel(subtitle, btn);
        sLabel->setStyleSheet("font-size: 12px; color: #666666; border: none; background: transparent;");
        sLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(tLabel);
        layout->addWidget(sLabel);
        btn->setLayout(layout);

        return btn;
    }

    void launchApp(const QString &exeName) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString exePath = QDir(appDir).filePath(exeName);

        if (!QFile::exists(exePath)) {
            // Check in subdirectory if placed in structured folders
            QString folderName = exeName;
            folderName.remove(".exe");
            exePath = QDir(appDir).filePath(folderName + "/" + exeName);
        }

        if (!QProcess::startDetached(exePath, QStringList())) {
            QMessageBox::warning(this, "Launch Failed", QString("Could not start %1.\nMake sure the binary is in the same folder as the launcher.").arg(exeName));
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::WindowText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::Text, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Button, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ButtonText, QColor(34, 34, 34));
    app.setPalette(lightPalette);

    SimpleOfficeLauncher launcher;
    launcher.show();
    return app.exec();
}

#include "main.moc"