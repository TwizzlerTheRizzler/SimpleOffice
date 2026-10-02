#include <QApplication>
#include <QMainWindow>
#include <QTableView>
#include <QAbstractTableModel>
#include <QHeaderView>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QStyleFactory>
#include <QPalette>
#include <QVector>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <set>
#include <cmath>

class SpreadsheetModel : public QAbstractTableModel {
    Q_OBJECT

public:
    SpreadsheetModel(int rows = 100, int cols = 26, QObject *parent = nullptr)
        : QAbstractTableModel(parent), m_rows(rows), m_cols(cols) {
        clearGrid();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_rows;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_cols;
    }

    QString getRawData(int row, int col) const {
        if (row >= 0 && row < m_rows && col >= 0 && col < m_cols) {
            return m_gridData[row][col];
        }
        return QString();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()) return QVariant();

        if (role == Qt::EditRole) {
            return m_gridData[index.row()][index.column()];
        }

        if (role == Qt::DisplayRole) {
            QString raw = m_gridData[index.row()][index.column()];
            if (raw.isEmpty()) return QVariant();
            return evaluateFormula(raw);
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
        if (index.isValid() && role == Qt::EditRole) {
            if (m_gridData[index.row()][index.column()] != value.toString()) {
                m_gridData[index.row()][index.column()] = value.toString();
                // Refresh full table display so all dependent formulas recalculate instantly
                emit dataChanged(this->index(0, 0), this->index(m_rows - 1, m_cols - 1), {Qt::DisplayRole, Qt::EditRole});
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        if (!index.isValid()) return Qt::NoItemFlags;
        return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (role != Qt::DisplayRole) return QVariant();

        if (orientation == Qt::Horizontal) {
            if (section < 26) {
                return QString(QChar('A' + section));
            }
            return QString("Col %1").arg(section + 1);
        } else {
            return QString::number(section + 1);
        }
    }

    void clearGrid() {
        beginResetModel();
        m_gridData.clear();
        m_gridData.resize(m_rows, QVector<QString>(m_cols, ""));
        endResetModel();
    }

    bool saveCsv(const QString &fileName) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream out(&file);

        int maxRow = -1;
        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                if (!m_gridData[r][c].isEmpty()) {
                    maxRow = r;
                    break;
                }
            }
        }

        for (int r = 0; r <= maxRow; ++r) {
            QStringList rowStrings;
            for (int c = 0; c < m_cols; ++c) {
                QString val = m_gridData[r][c];
                if (val.contains(',') || val.contains('"') || val.contains('\n')) {
                    val.replace('"', "\"\"");
                    val = QString("\"%1\"").arg(val);
                }
                rowStrings.append(val);
            }
            out << rowStrings.join(",") << "\n";
        }

        file.close();
        return true;
    }

    bool loadCsv(const QString &fileName) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        clearGrid();
        beginResetModel();

        QTextStream in(&file);
        int r = 0;

        while (!in.atEnd() && r < m_rows) {
            QString line = in.readLine();
            QStringList parsedLine = parseCsvLine(line);

            for (int c = 0; c < parsedLine.size() && c < m_cols; ++c) {
                m_gridData[r][c] = parsedLine[c];
            }
            r++;
        }

        file.close();
        endResetModel();
        return true;
    }

private:
    int m_rows;
    int m_cols;
    QVector<QVector<QString>> m_gridData;

    static QStringList parseCsvLine(const QString &line) {
        QStringList result;
        QString current;
        bool inQuotes = false;

        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line.at(i);
            if (ch == '"') {
                if (inQuotes && i + 1 < line.length() && line.at(i + 1) == '"') {
                    current += '"';
                    i++;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (ch == ',' && !inQuotes) {
                result.append(current);
                current.clear();
            } else {
                current += ch;
            }
        }
        result.append(current);
        return result;
    }

    // --- FORMULA PARSER & MATH ENGINE ---
    QString evaluateFormula(const QString &expr, std::set<QPair<int, int>> visited = {}) const {
        QString trimmed = expr.trimmed();
        bool isExplicit = trimmed.startsWith('=');
        if (isExplicit) {
            trimmed = trimmed.mid(1).trimmed();
        }

        if (trimmed.isEmpty()) return "";

        int pos = 0;
        bool ok = true;
        QString errorMsg;

        double result = parseExpression(trimmed, pos, visited, ok, errorMsg);

        if (!ok || pos < trimmed.length()) {
            if (isExplicit) {
                return errorMsg.isEmpty() ? "#ERROR!" : errorMsg;
            }
            return expr; // Return as plain text if it's not a valid math expression
        }

        if (std::floor(result) == result && std::abs(result) < 1e15) {
            return QString::number(static_cast<long long>(result));
        }
        return QString::number(result, 'g', 10);
    }

    double parseExpression(const QString &s, int &pos, std::set<QPair<int, int>> &visited, bool &ok, QString &errorMsg) const {
        double result = parseTerm(s, pos, visited, ok, errorMsg);
        while (ok && pos < s.length()) {
            skipWhitespace(s, pos);
            if (pos < s.length() && (s[pos] == '+' || s[pos] == '-')) {
                QChar op = s[pos++];
                double rhs = parseTerm(s, pos, visited, ok, errorMsg);
                if (op == '+') result += rhs;
                else result -= rhs;
            } else {
                break;
            }
        }
        return result;
    }

    double parseTerm(const QString &s, int &pos, std::set<QPair<int, int>> &visited, bool &ok, QString &errorMsg) const {
        double result = parseFactor(s, pos, visited, ok, errorMsg);
        while (ok && pos < s.length()) {
            skipWhitespace(s, pos);
            if (pos < s.length() && (s[pos] == '*' || s[pos] == '/')) {
                QChar op = s[pos++];
                double rhs = parseFactor(s, pos, visited, ok, errorMsg);
                if (op == '*') {
                    result *= rhs;
                } else {
                    if (rhs == 0) {
                        ok = false;
                        errorMsg = "#DIV/0!";
                        return 0;
                    }
                    result /= rhs;
                }
            } else {
                break;
            }
        }
        return result;
    }

    double parseFactor(const QString &s, int &pos, std::set<QPair<int, int>> &visited, bool &ok, QString &errorMsg) const {
        skipWhitespace(s, pos);
        if (pos >= s.length()) {
            ok = false;
            return 0;
        }

        if (s[pos] == '-') {
            pos++;
            return -parseFactor(s, pos, visited, ok, errorMsg);
        }
        if (s[pos] == '+') {
            pos++;
            return parseFactor(s, pos, visited, ok, errorMsg);
        }

        if (s[pos] == '(') {
            pos++;
            double val = parseExpression(s, pos, visited, ok, errorMsg);
            skipWhitespace(s, pos);
            if (pos < s.length() && s[pos] == ')') {
                pos++;
            } else {
                ok = false;
            }
            return val;
        }

        if (s[pos].isDigit() || s[pos] == '.') {
            int start = pos;
            while (pos < s.length() && (s[pos].isDigit() || s[pos] == '.')) {
                pos++;
            }
            bool numOk = false;
            double val = s.mid(start, pos - start).toDouble(&numOk);
            if (!numOk) ok = false;
            return val;
        }

        if (s[pos].isLetter()) {
            int start = pos;
            while (pos < s.length() && (s[pos].isLetter() || s[pos].isDigit())) {
                pos++;
            }
            QString token = s.mid(start, pos - start).toUpper();

            skipWhitespace(s, pos);
            if (pos < s.length() && s[pos] == '(' && (token == "SUM" || token == "AVG" || token == "AVERAGE")) {
                pos++;
                int argStart = pos;
                int parenCount = 1;
                while (pos < s.length() && parenCount > 0) {
                    if (s[pos] == '(') parenCount++;
                    else if (s[pos] == ')') parenCount--;
                    pos++;
                }
                QString arg = s.mid(argStart, pos - argStart - 1).trimmed();
                return evaluateRangeFunc(token, arg, visited, ok, errorMsg);
            }

            return evaluateCellRef(token, visited, ok, errorMsg);
        }

        ok = false;
        return 0;
    }

    static void skipWhitespace(const QString &s, int &pos) {
        while (pos < s.length() && s[pos].isSpace()) {
            pos++;
        }
    }

    double evaluateCellRef(const QString &ref, std::set<QPair<int, int>> &visited, bool &ok, QString &errorMsg) const {
        int r = -1, c = -1;
        if (!parseCellRef(ref, r, c)) {
            ok = false;
            return 0;
        }

        QPair<int, int> cellKey(r, c);
        if (visited.count(cellKey)) {
            ok = false;
            errorMsg = "#CIRCULAR!";
            return 0;
        }

        visited.insert(cellKey);
        QString rawVal = getRawData(r, c);
        if (rawVal.isEmpty()) return 0;

        bool isNum = false;
        double val = rawVal.toDouble(&isNum);
        if (isNum) return val;

        QString evalRes = evaluateFormula(rawVal, visited);
        if (evalRes.startsWith('#')) {
            ok = false;
            errorMsg = evalRes;
            return 0;
        }

        bool resNumOk = false;
        val = evalRes.toDouble(&resNumOk);
        if (!resNumOk) {
            ok = false;
            errorMsg = "#VALUE!";
            return 0;
        }
        return val;
    }

    bool parseCellRef(const QString &ref, int &row, int &col) const {
        QString colStr, rowStr;
        for (QChar ch : ref) {
            if (ch.isLetter()) colStr += ch;
            else if (ch.isDigit()) rowStr += ch;
            else return false;
        }

        if (colStr.isEmpty() || rowStr.isEmpty()) return false;

        int c = 0;
        for (QChar ch : colStr.toUpper()) {
            c = c * 26 + (ch.toLatin1() - 'A' + 1);
        }
        col = c - 1;
        row = rowStr.toInt() - 1;

        return (row >= 0 && row < m_rows && col >= 0 && col < m_cols);
    }

    double evaluateRangeFunc(const QString &func, const QString &rangeStr, std::set<QPair<int, int>> &visited, bool &ok, QString &errorMsg) const {
        QStringList parts = rangeStr.split(':');
        if (parts.size() != 2) {
            ok = false;
            errorMsg = "#REF!";
            return 0;
        }

        int r1, c1, r2, c2;
        if (!parseCellRef(parts[0].trimmed(), r1, c1) ||
            !parseCellRef(parts[1].trimmed(), r2, c2)) {
            ok = false;
            errorMsg = "#REF!";
            return 0;
        }

        int minR = qMin(r1, r2), maxR = qMax(r1, r2);
        int minC = qMin(c1, c2), maxC = qMax(c1, c2);

        double sum = 0;
        int count = 0;

        for (int r = minR; r <= maxR; ++r) {
            for (int c = minC; c <= maxC; ++c) {
                QString cellName = QString("%1%2").arg(QChar('A' + c)).arg(r + 1);
                double val = evaluateCellRef(cellName, visited, ok, errorMsg);
                if (!ok) return 0;
                sum += val;
                count++;
            }
        }

        if (func == "SUM") return sum;
        if (func == "AVG" || func == "AVERAGE") return count > 0 ? sum / count : 0;

        ok = false;
        return 0;
    }
};

class SimpleSheet : public QMainWindow {
    Q_OBJECT

public:
    SimpleSheet(QWidget *parent = nullptr) : QMainWindow(parent), m_isDirty(false) {
        model = new SpreadsheetModel(100, 26, this);
        tableView = new QTableView(this);
        tableView->setModel(model);

        tableView->horizontalHeader()->setDefaultSectionSize(85);
        tableView->verticalHeader()->setDefaultSectionSize(26);
        tableView->setStyleSheet(
            "QTableView {"
            "  background-color: #ffffff;"
            "  gridline-color: #dcdcdc;"
            "  color: #111111;"
            "  selection-background-color: #e3f2fd;"
            "  selection-color: #000000;"
            "}"
            "QHeaderView::section {"
            "  background-color: #f0f0f0;"
            "  color: #333333;"
            "  border: 1px solid #dcdcdc;"
            "  font-weight: bold;"
            "}"
        );

        setCentralWidget(tableView);
        setupToolBar();
        setupStatusBar();
        updateWindowTitle();

        connect(model, &SpreadsheetModel::dataChanged, this, [this]() {
            setDirty(true);
        });

        resize(1000, 700);
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
    SpreadsheetModel *model;
    QTableView *tableView;
    QLabel *statusLabel;
    QString m_currentFilePath;
    bool m_isDirty;

    void setupToolBar() {
        QToolBar *toolbar = addToolBar("Main Toolbar");
        toolbar->setMovable(false);
        toolbar->setStyleSheet(
            "QToolBar { background-color: #f5f5f5; border-bottom: 1px solid #dcdcdc; padding: 2px; }"
            "QToolButton { color: #222222; background: transparent; padding: 4px 8px; font-weight: bold; }"
            "QToolButton:hover { background-color: #e0e0e0; }"
        );

        QAction *newAct = toolbar->addAction("New");
        QAction *openAct = toolbar->addAction("Open");
        QAction *saveAct = toolbar->addAction("Save");

        connect(newAct, &QAction::triggered, this, &SimpleSheet::newFile);
        connect(openAct, &QAction::triggered, this, &SimpleSheet::openFile);
        connect(saveAct, &QAction::triggered, this, &SimpleSheet::saveFile);
    }

    void setupStatusBar() {
        statusLabel = new QLabel("Ready", this);
        statusBar()->setStyleSheet(
            "QStatusBar { background-color: #f5f5f5; color: #333333; border-top: 1px solid #dcdcdc; }"
        );
        statusBar()->setSizeGripEnabled(false);
        statusBar()->addWidget(statusLabel);
    }

    void setDirty(bool dirty) {
        m_isDirty = dirty;
        setWindowModified(dirty);
        updateWindowTitle();
    }

    void updateWindowTitle() {
        QString fileName = m_currentFilePath.isEmpty() ? "Untitled" : QFileInfo(m_currentFilePath).fileName();
        setWindowTitle(QString("%1[*] - SimpleSheet").arg(fileName));
    }

    bool maybeSave() {
        if (!m_isDirty) return true;

        QMessageBox::StandardButton ret = QMessageBox::warning(
            this, "SimpleSheet",
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

    void newFile() {
        if (maybeSave()) {
            model->clearGrid();
            m_currentFilePath.clear();
            setDirty(false);
            statusLabel->setText("New spreadsheet created.");
        }
    }

    void openFile() {
        if (maybeSave()) {
            QString fileName = QFileDialog::getOpenFileName(
                this, "Open CSV File", "", "CSV Files (*.csv);;All Files (*)"
            );
            if (!fileName.isEmpty()) {
                if (model->loadCsv(fileName)) {
                    m_currentFilePath = fileName;
                    setDirty(false);
                    statusLabel->setText("Loaded: " + QFileInfo(fileName).fileName());
                } else {
                    QMessageBox::critical(this, "Error", "Could not open file.");
                }
            }
        }
    }

    bool saveFile() {
        if (m_currentFilePath.isEmpty()) {
            return saveFileAs();
        }
        if (model->saveCsv(m_currentFilePath)) {
            setDirty(false);
            statusLabel->setText("Saved: " + QFileInfo(m_currentFilePath).fileName());
            return true;
        }
        QMessageBox::critical(this, "Error", "Could not save file.");
        return false;
    }

    bool saveFileAs() {
        QString fileName = QFileDialog::getSaveFileName(
            this, "Save CSV File", "Spreadsheet.csv", "CSV Files (*.csv);;All Files (*)"
        );
        if (fileName.isEmpty()) return false;

        m_currentFilePath = fileName;
        return saveFile();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::WindowText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::Text, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Button, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::ButtonText, QColor(34, 34, 34));
    lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    app.setPalette(lightPalette);

    SimpleSheet window;
    window.show();
    return app.exec();
}

#include "main.moc"