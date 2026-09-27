#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QStatusBar>
#include <QLabel>
#include <QTextStream>
#include <QFile>
#include <QRegularExpression>
#include <QTextDocumentWriter>

class WordProcessor : public QMainWindow {
public:
    WordProcessor(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Simple Word Processor");
        resize(900, 650);

        editor = new QTextEdit(this);
        setCentralWidget(editor);

        setupToolBar();
        setupStatusBar();

        connect(editor, &QTextEdit::textChanged, this, &WordProcessor::updateWordCount);
        connect(editor, &QTextEdit::cursorPositionChanged, this, &WordProcessor::updateFormatToolbar);
    }

private:
    QTextEdit *editor;
    QString currentFilePath;
    QLabel *statusLabel;

    QAction *actBold;
    QAction *actItalic;
    QAction *actUnderline;
    QFontComboBox *fontCombo;
    QSpinBox *sizeSpinBox;

    void setupToolBar() {
        QToolBar *toolbar = addToolBar("Main Toolbar");
        toolbar->setMovable(false);

        // File Operations
        QAction *actNew = toolbar->addAction("New");
        connect(actNew, &QAction::triggered, this, [this]() {
            editor->clear();
            currentFilePath.clear();
            setWindowTitle("Simple Word Processor - New Document");
        });

        QAction *actOpen = toolbar->addAction("Open");
        connect(actOpen, &QAction::triggered, this, &WordProcessor::openFile);

        QAction *actSave = toolbar->addAction("Save");
        connect(actSave, &QAction::triggered, this, &WordProcessor::saveFile);

        toolbar->addSeparator();

        // Text Formatting
        actBold = toolbar->addAction("B");
        actBold->setCheckable(true);
        connect(actBold, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
            editor->mergeCurrentCharFormat(fmt);
        });

        actItalic = toolbar->addAction("I");
        actItalic->setCheckable(true);
        connect(actItalic, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontItalic(checked);
            editor->mergeCurrentCharFormat(fmt);
        });

        actUnderline = toolbar->addAction("U");
        actUnderline->setCheckable(true);
        connect(actUnderline, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontUnderline(checked);
            editor->mergeCurrentCharFormat(fmt);
        });

        toolbar->addSeparator();

        // Alignment
        QAction *actLeft = toolbar->addAction("Left");
        connect(actLeft, &QAction::triggered, this, [this]() { editor->setAlignment(Qt::AlignLeft); });

        QAction *actCenter = toolbar->addAction("Center");
        connect(actCenter, &QAction::triggered, this, [this]() { editor->setAlignment(Qt::AlignCenter); });

        QAction *actRight = toolbar->addAction("Right");
        connect(actRight, &QAction::triggered, this, [this]() { editor->setAlignment(Qt::AlignRight); });

        toolbar->addSeparator();

        // Font Family Selector
        fontCombo = new QFontComboBox(this);
        connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) {
            QTextCharFormat fmt;
            fmt.setFont(f);
            editor->mergeCurrentCharFormat(fmt);
        });
        toolbar->addWidget(fontCombo);

        toolbar->addSeparator();

        // Google Docs Style Font Size Control (- Number +)
        QAction *actDecreaseFont = toolbar->addAction("-");

        sizeSpinBox = new QSpinBox(this);
        sizeSpinBox->setRange(1, 144);
        sizeSpinBox->setValue(12);
        sizeSpinBox->setFixedWidth(45);
        sizeSpinBox->setAlignment(Qt::AlignCenter);
        sizeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        toolbar->addWidget(sizeSpinBox);

        QAction *actIncreaseFont = toolbar->addAction("+");

        connect(actDecreaseFont, &QAction::triggered, this, [this]() {
            if (sizeSpinBox->value() > sizeSpinBox->minimum()) {
                sizeSpinBox->setValue(sizeSpinBox->value() - 1);
            }
        });

        connect(actIncreaseFont, &QAction::triggered, this, [this]() {
            if (sizeSpinBox->value() < sizeSpinBox->maximum()) {
                sizeSpinBox->setValue(sizeSpinBox->value() + 1);
            }
        });

        connect(sizeSpinBox, &QSpinBox::valueChanged, this, [this](int newSize) {
            QTextCharFormat fmt;
            fmt.setFontPointSize(newSize);
            editor->mergeCurrentCharFormat(fmt);
        });
    }

    void updateFormatToolbar() {
        QTextCharFormat fmt = editor->currentCharFormat();

        actBold->blockSignals(true);
        actItalic->blockSignals(true);
        actUnderline->blockSignals(true);
        fontCombo->blockSignals(true);
        sizeSpinBox->blockSignals(true);

        actBold->setChecked(fmt.fontWeight() == QFont::Bold);
        actItalic->setChecked(fmt.fontItalic());
        actUnderline->setChecked(fmt.fontUnderline());
        fontCombo->setCurrentFont(fmt.font());

        if (fmt.fontPointSize() > 0) {
            sizeSpinBox->setValue(qRound(fmt.fontPointSize()));
        } else {
            sizeSpinBox->setValue(12);
        }

        actBold->blockSignals(false);
        actItalic->blockSignals(false);
        actUnderline->blockSignals(false);
        fontCombo->blockSignals(false);
        sizeSpinBox->blockSignals(false);
    }

    void setupStatusBar() {
        statusLabel = new QLabel("Words: 0 | Characters: 0", this);
        statusBar()->setStyleSheet("QStatusBar::item { border: none; }");
        statusBar()->setSizeGripEnabled(false);
        statusBar()->addWidget(statusLabel);
    }

    void updateWordCount() {
        QString text = editor->toPlainText().trimmed();
        qsizetype words = text.isEmpty() ? 0 : text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
        qsizetype chars = text.length();
        statusLabel->setText(QString("Words: %1 | Characters: %2").arg(words).arg(chars));
    }

    void openFile() {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open File",
            "",
            "All Supported (*.html *.htm *.txt *.odt);;OpenDocument Text (*.odt);;HTML Files (*.html *.htm);;Text Files (*.txt)"
        );

        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly)) {
                if (fileName.endsWith(".html", Qt::CaseInsensitive) || fileName.endsWith(".htm", Qt::CaseInsensitive)) {
                    QTextStream in(&file);
                    editor->setHtml(in.readAll());
                } else if (fileName.endsWith(".txt", Qt::CaseInsensitive)) {
                    QTextStream in(&file);
                    editor->setPlainText(in.readAll());
                } else {
                    QTextStream in(&file);
                    editor->setPlainText(in.readAll());
                }
                currentFilePath = fileName;
                setWindowTitle("Simple Word Processor - " + fileName);
            }
        }
    }

    void saveFile() {
        if (currentFilePath.isEmpty()) {
            currentFilePath = QFileDialog::getSaveFileName(
                this,
                "Save File",
                "",
                "OpenDocument Text (*.odt);;HTML Files (*.html);;Text Files (*.txt)"
            );
        }

        if (!currentFilePath.isEmpty()) {
            if (currentFilePath.endsWith(".odt", Qt::CaseInsensitive)) {
                QTextDocumentWriter writer(currentFilePath, "ODF");
                writer.write(editor->document());
            } else {
                QFile file(currentFilePath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    if (currentFilePath.endsWith(".txt", Qt::CaseInsensitive)) {
                        out << editor->toPlainText();
                    } else {
                        out << editor->toHtml();
                    }
                }
            }
            setWindowTitle("Simple Word Processor - " + currentFilePath);
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WordProcessor window;
    window.show();
    return app.exec();
}