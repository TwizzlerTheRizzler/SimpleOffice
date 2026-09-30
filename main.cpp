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
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QIcon>
#include <QKeySequence>

class WordProcessor : public QMainWindow {
public:
    WordProcessor(QWidget *parent = nullptr) : QMainWindow(parent) {
        updateWindowTitle();
        setWindowIcon(QIcon(":/icon.ico"));
        resize(900, 650);

        editor = new QTextEdit(this);
        setCentralWidget(editor);

        setupToolBar();
        setupStatusBar();

        connect(editor, &QTextEdit::textChanged, this, &WordProcessor::updateWordCount);
        connect(editor, &QTextEdit::cursorPositionChanged, this, &WordProcessor::updateFormatToolbar);
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (maybeSave()) {
            event->accept();
        } else {
            event->ignore();
        }
    }

private:
    QTextEdit *editor;
    QString currentFilePath;
    QLabel *statusLabel;

    QAction *actUndo;
    QAction *actRedo;
    QAction *actBold;
    QAction *actItalic;
    QAction *actUnderline;
    QFontComboBox *fontCombo;
    QSpinBox *sizeSpinBox;

    void updateWindowTitle() {
        if (currentFilePath.isEmpty()) {
            setWindowTitle("New Document - Simple Word Processor");
        } else {
            QFileInfo fileInfo(currentFilePath);
            setWindowTitle(fileInfo.fileName() + " - Simple Word Processor");
        }
    }

    bool maybeSave() {
        if (!editor->document()->isModified()) {
            return true;
        }

        QMessageBox::StandardButton ret = QMessageBox::warning(
            this,
            "Unsaved Changes",
            "The document has been modified.\nDo you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (ret == QMessageBox::Save) {
            return saveFile();
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
        return true;
    }

    void setupToolBar() {
        QToolBar *toolbar = addToolBar("Main Toolbar");
        toolbar->setMovable(false);

        // Styling: Enabled buttons show white text; disabled buttons stay grey
        toolbar->setStyleSheet(
            "QToolButton { color: #ffffff; background: transparent; padding: 3px 6px; border-radius: 3px; }"
            "QToolButton:hover { background-color: #3e3e42; }"
            "QToolButton:disabled { color: #666666; }"
        );

        // File Operations
        QAction *actNew = toolbar->addAction("New");
        connect(actNew, &QAction::triggered, this, [this]() {
            if (maybeSave()) {
                editor->clear();
                editor->document()->setModified(false);
                currentFilePath.clear();
                updateWindowTitle();
            }
        });

        QAction *actOpen = toolbar->addAction("Open");
        connect(actOpen, &QAction::triggered, this, [this]() { openFile(); });

        QAction *actSave = toolbar->addAction("Save");
        connect(actSave, &QAction::triggered, this, [this]() { saveFile(); });

        toolbar->addSeparator();

        // Undo & Redo Operations
        actUndo = toolbar->addAction("Undo");
        actUndo->setShortcut(QKeySequence::Undo);
        actUndo->setEnabled(false);
        connect(actUndo, &QAction::triggered, editor, &QTextEdit::undo);

        actRedo = toolbar->addAction("Redo");
        actRedo->setShortcut(QKeySequence::Redo);
        actRedo->setEnabled(false);
        connect(actRedo, &QAction::triggered, editor, &QTextEdit::redo);

        connect(editor, &QTextEdit::undoAvailable, actUndo, &QAction::setEnabled);
        connect(editor, &QTextEdit::redoAvailable, actRedo, &QAction::setEnabled);

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

        // Font Size Control
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

        qreal ptSize = fmt.fontPointSize();
        if (ptSize > 0) {
            sizeSpinBox->setValue(qRound(ptSize));
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
        if (!maybeSave()) return;

        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open File",
            "",
            "Supported Files (*.html *.htm *.md *.txt);;HTML Files (*.html *.htm);;Markdown Files (*.md);;Text Files (*.txt)"
        );

        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();

                if (fileName.endsWith(".html", Qt::CaseInsensitive) || fileName.endsWith(".htm", Qt::CaseInsensitive)) {
                    editor->setHtml(content);
                } else if (fileName.endsWith(".md", Qt::CaseInsensitive)) {
                    editor->setMarkdown(content);
                } else {
                    editor->setPlainText(content);
                }

                currentFilePath = fileName;
                editor->document()->setModified(false);
                updateWindowTitle();
            }
        }
    }

    bool saveFile() {
        if (currentFilePath.isEmpty()) {
            currentFilePath = QFileDialog::getSaveFileName(
                this,
                "Save File",
                "",
                "HTML Files (*.html);;Markdown Files (*.md);;Text Files (*.txt)"
            );
            if (currentFilePath.isEmpty()) {
                return false;
            }
        }

        QFile file(currentFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (currentFilePath.endsWith(".html", Qt::CaseInsensitive) || currentFilePath.endsWith(".htm", Qt::CaseInsensitive)) {
                out << editor->toHtml();
            } else if (currentFilePath.endsWith(".md", Qt::CaseInsensitive)) {
                out << editor->toMarkdown();
            } else {
                out << editor->toPlainText();
            }
            file.close();
            editor->document()->setModified(false);
            updateWindowTitle();
            return true;
        }

        return false;
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WordProcessor window;
    window.show();
    return app.exec();
}