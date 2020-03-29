#pragma once 
#include "pathBrowser.h"

#include <QtWidgets>

class PrimExportWidget : public QGroupBox {
    Q_OBJECT

public:
    PrimExportWidget(QWidget* parent = nullptr);

    void doExport();

    QComboBox* cbPrimIds;
    QTreeView* tvPrimReferences;
    QCheckBox* cbExportTextures;
    PathBrowserWidget* exportDirectory;
    QPushButton* pbExportModel;

public slots:
    void updateResourceDependencyTree(const QString& text);
    void exportModel();

signals:
    void exportStarted();
    void exportFinished();
};