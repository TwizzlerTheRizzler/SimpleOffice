#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
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
#include <QKeyEvent>
#include <QList>
#include <QTimer>
#include <QTextDocumentWriter>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QStyleFactory>
#include <QPalette>

class WordProcessor : public QMainWindow {
public:
    WordProcessor(QWidget *parent = nullptr) : QMainWindow(parent) {
    setupPageCanvas();
    setupToolBar();
    setupStatusBar();

    updateWindowTitle();
    setWindowIcon(QIcon(":/icon.ico"));
    resize(1000, 750);

    updatePageSize();
    saveUndoState();

    connect(editor, &QTextEdit::textChanged, this, &WordProcessor::updateWordCount);
    connect(editor, &QTextEdit::cursorPositionChanged, this, &WordProcessor::updateFormatToolbar);

    // Reset the modified flag so a fresh launch starts completely clean
    editor->document()->setModified(false);
}

protected:
    void closeEvent(QCloseEvent *event) override {
        if (maybeSave()) {
            event->accept();
        } else {
            event->ignore();
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == editor && event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();
            QString text = keyEvent->text();

            if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter ||
                key == Qt::Key_Tab || (!text.isEmpty() && text.at(0).isPunct())) {
                QTimer::singleShot(0, this, [this]() { saveUndoState(); });
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }

private:
    QTextEdit *editor;
    QScrollArea *scrollArea;
    QWidget *pageContainer;

    QDoubleSpinBox *pageWidthSpinBox;
    QDoubleSpinBox *pageHeightSpinBox;

    QString currentFilePath;
    QLabel *statusLabel;

    QAction *actUndo;
    QAction *actRedo;
    QAction *actBold;
    QAction *actItalic;
    QAction *actUnderline;
    QFontComboBox *fontCombo;
    QSpinBox *sizeSpinBox;

    QList<QString> undoStack;
    QList<QString> redoStack;
    const int MAX_UNDO_LIMIT = 15;
    bool isUndoRedoOperation = false;

    void setupPageCanvas() {
        // Off-white workspace background
        scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setAlignment(Qt::AlignCenter);
        scrollArea->setStyleSheet("QScrollArea { background-color: #e8e8e8; border: none; }");

        pageContainer = new QWidget(scrollArea);
        pageContainer->setStyleSheet("background-color: #e8e8e8;");

        QVBoxLayout *containerLayout = new QVBoxLayout(pageContainer);
        containerLayout->setAlignment(Qt::AlignCenter);
        containerLayout->setContentsMargins(40, 40, 40, 40);

        // Pure white paper sheet
        editor = new QTextEdit(pageContainer);
        editor->installEventFilter(this);
        editor->setStyleSheet(
            "QTextEdit {"
            "   background-color: #ffffff;"
            "   color: #111111;"
            "   border: 1px solid #cccccc;"
            "   padding: 20px;"
            "}"
        );
        editor->document()->setDocumentMargin(20);

        containerLayout->addWidget(editor);
        scrollArea->setWidget(pageContainer);
        setCentralWidget(scrollArea);
    }

    void updatePageSize() {
        double widthCm = pageWidthSpinBox->value();
        double heightCm = pageHeightSpinBox->value();

        QScreen *screen = QGuiApplication::primaryScreen();
        double dpiX = screen ? screen->logicalDotsPerInchX() : 96.0;
        double dpiY = screen ? screen->logicalDotsPerInchY() : 96.0;

        int widthPx = qRound(widthCm * (dpiX / 2.54));
        int heightPx = qRound(heightCm * (dpiY / 2.54));

        editor->setFixedSize(widthPx, heightPx);
    }

    void saveUndoState() {
        if (isUndoRedoOperation) return;

        QString currentState = editor->toHtml();
        if (!undoStack.isEmpty() && undoStack.last() == currentState) {
            return;
        }

        undoStack.append(currentState);
        if (undoStack.size() > MAX_UNDO_LIMIT) {
            undoStack.removeFirst();
        }
        redoStack.clear();
        updateUndoRedoStates();
    }

    void undo() {
        if (undoStack.size() <= 1) return;

        isUndoRedoOperation = true;
        redoStack.append(editor->toHtml());
        undoStack.removeLast();

        QString previousState = undoStack.last();
        editor->setHtml(previousState);
        editor->moveCursor(QTextCursor::End);

        isUndoRedoOperation = false;
        updateUndoRedoStates();
    }

    void redo() {
        if (redoStack.isEmpty()) return;

        isUndoRedoOperation = true;

        QString nextState = redoStack.takeLast();
        undoStack.append(nextState);
        if (undoStack.size() > MAX_UNDO_LIMIT) {
            undoStack.removeFirst();
        }

        editor->setHtml(nextState);
        editor->moveCursor(QTextCursor::End);

        isUndoRedoOperation = false;
        updateUndoRedoStates();
    }

    void updateUndoRedoStates() {
        actUndo->setEnabled(undoStack.size() > 1);
        actRedo->setEnabled(!redoStack.isEmpty());
    }

    void updateWindowTitle() {
        if (currentFilePath.isEmpty()) {
            setWindowTitle("New Document - SimpleWrite");
        } else {
            QFileInfo fileInfo(currentFilePath);
            setWindowTitle(fileInfo.fileName() + " - SimpleWrite");
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

        // Explicit light-mode styling for toolbar and input fields
        toolbar->setStyleSheet(
            "QToolBar { background-color: #f5f5f5; border-bottom: 1px solid #dcdcdc; padding: 2px; }"
            "QToolButton { color: #222222; background: transparent; padding: 4px 8px; border-radius: 3px; font-weight: bold; font-size: 13px; }"
            "QToolButton:hover { background-color: #e0e0e0; }"
            "QToolButton:checked { background-color: #d0d0d0; font-weight: bold; }"
            "QToolButton:disabled { color: #a0a0a0; }"
            "QLabel { color: #333333; padding-left: 4px; padding-right: 2px; }"
            "QSpinBox, QDoubleSpinBox, QFontComboBox {"
            "   background-color: #ffffff;"
            "   color: #222222;"
            "   border: 1px solid #cccccc;"
            "   border-radius: 3px;"
            "   padding: 2px 4px;"
            "}"
            "QSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
            "   background-color: #e8e8e8;"
            "   border: none;"
            "}"
        );

        // File Operations
        QAction *actNew = toolbar->addAction("New");
        connect(actNew, &QAction::triggered, this, [this]() {
            if (maybeSave()) {
                editor->clear();
                editor->document()->setModified(false);
                currentFilePath.clear();
                undoStack.clear();
                redoStack.clear();
                saveUndoState();
                updateWindowTitle();
            }
        });

        QAction *actOpen = toolbar->addAction("Open");
        connect(actOpen, &QAction::triggered, this, [this]() { openFile(); });

        QAction *actSave = toolbar->addAction("Save");
        connect(actSave, &QAction::triggered, this, [this]() { saveFile(); });

        toolbar->addSeparator();

        // Page Dimension Controls (cm)
        QLabel *lblPage = new QLabel("Page Size (cm):", this);
        toolbar->addWidget(lblPage);

        pageWidthSpinBox = new QDoubleSpinBox(this);
        pageWidthSpinBox->setRange(5.0, 100.0);
        pageWidthSpinBox->setValue(21.0);
        pageWidthSpinBox->setSingleStep(0.5);
        pageWidthSpinBox->setSuffix(" cm");
        pageWidthSpinBox->setFixedWidth(75);
        toolbar->addWidget(pageWidthSpinBox);

        QLabel *lblX = new QLabel("x", this);
        toolbar->addWidget(lblX);

        pageHeightSpinBox = new QDoubleSpinBox(this);
        pageHeightSpinBox->setRange(5.0, 100.0);
        pageHeightSpinBox->setValue(29.7);
        pageHeightSpinBox->setSingleStep(0.5);
        pageHeightSpinBox->setSuffix(" cm");
        pageHeightSpinBox->setFixedWidth(75);
        toolbar->addWidget(pageHeightSpinBox);

        connect(pageWidthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updatePageSize();
        });
        connect(pageHeightSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updatePageSize();
        });

        toolbar->addSeparator();

        // Undo & Redo Operations
        actUndo = toolbar->addAction("Undo");
        actUndo->setShortcut(QKeySequence::Undo);
        actUndo->setEnabled(false);
        connect(actUndo, &QAction::triggered, this, &WordProcessor::undo);

        actRedo = toolbar->addAction("Redo");
        actRedo->setShortcut(QKeySequence::Redo);
        actRedo->setEnabled(false);
        connect(actRedo, &QAction::triggered, this, &WordProcessor::redo);

        toolbar->addSeparator();

        // Text Formatting
        actBold = toolbar->addAction("B");
        actBold->setCheckable(true);
        connect(actBold, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
            editor->mergeCurrentCharFormat(fmt);
            saveUndoState();
        });

        actItalic = toolbar->addAction("I");
        actItalic->setCheckable(true);
        connect(actItalic, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontItalic(checked);
            editor->mergeCurrentCharFormat(fmt);
            saveUndoState();
        });

        actUnderline = toolbar->addAction("U");
        actUnderline->setCheckable(true);
        connect(actUnderline, &QAction::toggled, this, [this](bool checked) {
            QTextCharFormat fmt;
            fmt.setFontUnderline(checked);
            editor->mergeCurrentCharFormat(fmt);
            saveUndoState();
        });

        toolbar->addSeparator();

        // Alignment
        QAction *actLeft = toolbar->addAction("Left");
        connect(actLeft, &QAction::triggered, this, [this]() {
            editor->setAlignment(Qt::AlignLeft);
            saveUndoState();
        });

        QAction *actCenter = toolbar->addAction("Center");
        connect(actCenter, &QAction::triggered, this, [this]() {
            editor->setAlignment(Qt::AlignCenter);
            saveUndoState();
        });

        QAction *actRight = toolbar->addAction("Right");
        connect(actRight, &QAction::triggered, this, [this]() {
            editor->setAlignment(Qt::AlignRight);
            saveUndoState();
        });

        toolbar->addSeparator();

        // Font Family Selector
        fontCombo = new QFontComboBox(this);
        connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) {
            QTextCharFormat fmt;
            fmt.setFont(f);
            editor->mergeCurrentCharFormat(fmt);
            saveUndoState();
        });
        toolbar->addWidget(fontCombo);

        toolbar->addSeparator();

        // Font Size Control with thick Unicode minus sign ("−")
        QAction *actDecreaseFont = toolbar->addAction("−");

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
            saveUndoState();
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
        statusBar()->setStyleSheet(
            "QStatusBar { background-color: #f5f5f5; color: #333333; border-top: 1px solid #dcdcdc; }"
            "QStatusBar::item { border: none; }"
        );
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
                undoStack.clear();
                redoStack.clear();
                saveUndoState();
                updateWindowTitle();
            }
        }
    }

    bool saveFile() {
        if (currentFilePath.isEmpty()) {
            QString selectedFilter = "OpenDocument Text (*.odt)";

            currentFilePath = QFileDialog::getSaveFileName(
                this,
                "Save File",
                "Untitled.odt",
                "OpenDocument Text (*.odt);;HTML Files (*.html *.htm);;Markdown Files (*.md);;Text Files (*.txt)",
                &selectedFilter
            );

            if (currentFilePath.isEmpty()) {
                return false;
            }

            if (!currentFilePath.contains('.')) {
                currentFilePath += ".odt";
            }
        }

        if (currentFilePath.endsWith(".odt", Qt::CaseInsensitive)) {
            QTextDocumentWriter writer(currentFilePath, "ODF");
            if (writer.write(editor->document())) {
                editor->document()->setModified(false);
                updateWindowTitle();
                return true;
            }
            return false;
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

    // Force Fusion style and explicit light palette regardless of OS dark mode
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::WindowText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Text, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Button, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ButtonText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::BrightText, Qt::red);
    lightPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(lightPalette);

    WordProcessor window;
    window.show();
    return app.exec();
}