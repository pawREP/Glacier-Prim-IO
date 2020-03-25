#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QtWidgets>
#include <QDir>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

class ConsoleWidget : public QWidget {
    Q_OBJECT

private:
    QTextEdit* text;

public:
    ConsoleWidget(QWidget* parent = nullptr);
    ~ConsoleWidget();

    void print(const char* message, const QColor& color);
    void print(const std::string& message, const QColor& color);
    void print(const QString& message, const QColor& color);
};

class Console {
    ConsoleWidget* console_widget;

public:
    static Console& instance();
    void setDestinationWidget(ConsoleWidget* widget);

    template<typename T>
    void print(T msg, const QColor& color = Qt::white) {
        if (console_widget)
            console_widget->print(msg, color);
    }

};

class FooterBar : public QWidget {
    Q_OBJECT
public:
    FooterBar(QWidget* parent);
    ~FooterBar();

private slots:
    void exitClicked();
};

enum class PathBrowserType {
    SAVE_FILE,
    OPEN_FILE,
    OPEN_DIRECTORY
};

class PathBrowserWidget : public QWidget {
    Q_OBJECT

public:
    PathBrowserWidget(PathBrowserType type, const QString& label, const QString& filter = "", QWidget* parent = nullptr);
    ~PathBrowserWidget();

    QString path() const;
    void setPath(const QString& path);

private:
    PathBrowserType type;
    QString filter;

    QLineEdit* lePath;
    QPushButton* pbBrowse;

private slots:
    void browse();

signals:
    void pathChanged();
};

class PrimExportWidget : public QWidget {
    Q_OBJECT

public:
    PrimExportWidget(QWidget* parent = nullptr);
    ~PrimExportWidget();

    void doExport();

    //Export group
    QComboBox* cbPrimIds;
    QTreeView* tvPrimReferences;
    QCheckBox* cbExportTextures;
    PathBrowserWidget* exportDirectory;
    QPushButton* pbExportModel;

public slots:
    void updateResourceDependencyTree(const QString& text);
    void exportModel();
};

class GltfImportOptions : public QGroupBox {
    Q_OBJECT

public:
    GltfImportOptions(QWidget* parent = nullptr);
    ~GltfImportOptions();

    bool importTextures();
    bool useMaxLODRange();
    bool useCustomMaterialId();
    int materialId();

private:
    QCheckBox* cbImportTextures;
    QCheckBox* cbUseMaxLODRange;
    QCheckBox* cbUseCustomMaterialId;
    QSpinBox* sbMaterialId;

private slots:
    void materialIdOverrideChecked(int);
};

class LabeledLineEdit : public QWidget {
    Q_OBJECT

public:
    LabeledLineEdit(const QString& label, const QString& toolTip, const QString& placeholderText, QWidget* parent = nullptr);
    ~LabeledLineEdit();

    QString text() const;

private:
    QLineEdit* line;
};

class DeletionList : public LabeledLineEdit {
    Q_OBJECT

public:
    DeletionList(QWidget* parent = nullptr);
    ~DeletionList();

    std::vector<uint64_t> deletionList() const;
};

class GltfImportWidget : public QWidget {
    Q_OBJECT

public:
    GltfImportWidget(QWidget* parent = nullptr);
    ~GltfImportWidget();

private:
    PathBrowserWidget* gltfBrowser;
    QTextEdit* teGltfInfo;
    GltfImportOptions* options;
    DeletionList* deletionList;
    QLineEdit* leDeletionList;
    PathBrowserWidget* patchFileBrowser;
    QPushButton* pbImport;

    void doImport();

private slots:
    void gltfPathUpdated();
    void importGltf();

};

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    PathBrowserWidget* runtimeDirectory;

    PrimExportWidget* primExportWidget;
    GltfImportWidget* gltfImportWidget;
    ConsoleWidget* console;

    QLayout* createImportLayout();
    QLayout* createDebugOutConsoleLayout();

    //other
    void initilizeWindowsComponents();
};

#endif // MAINWINDOW_H
