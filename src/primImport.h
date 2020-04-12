#pragma once
#include "pathBrowser.h"
#include "Console.h"

#include <QtWidgets>

class LabeledLineEdit : public QWidget {
    Q_OBJECT

public:
    LabeledLineEdit(const QString& label, const QString& toolTip, const QString& placeholderText, QWidget* parent = nullptr);

    QString text() const;

private:
    QLineEdit* line;
};

class DeletionList : public LabeledLineEdit {
    Q_OBJECT

public:
    DeletionList(QWidget* parent = nullptr);

    std::vector<uint64_t> deletionList() const;
};

class GltfImportOptions : public QGroupBox {
    Q_OBJECT

public:
    GltfImportOptions(QWidget* parent = nullptr);

    bool importTextures();
    bool useMaxLODRange();
    bool useCustomMaterialId();
    bool useOriginalBoneInfo();
    bool doInvertNormalsX();
    bool doInvertNormalsY();
    bool doInvertNormalsZ();
    bool autoOrientNormals();
    int materialId();

private:
    QCheckBox* cbImportTextures;
    QCheckBox* cbUseMaxLODRange;
    QCheckBox* cbUseCustomMaterialId;
    QCheckBox* cbUseOriginalBoneInfo;

    QCheckBox* cbInvertNormalX;
    QCheckBox* cbInvertNormalY;
    QCheckBox* cbInvertNormalZ;
    QCheckBox* cbAutoOrientNormal;

    QSpinBox* sbMaterialId;

private slots:
    void materialIdOverrideChecked(int);
};

class GltfImportWidget : public QWidget {
    Q_OBJECT

public:
    GltfImportWidget(QWidget* parent = nullptr);

private:
    PathBrowserWidget* gltfBrowser;
    QTextEdit* teGltfInfo;
    GltfImportOptions* options;
    DeletionList* deletionList;
    PathBrowserWidget* patchFileBrowser;
    QPushButton* pbImport;

    void doImport();

private slots:
    void gltfPathUpdated();
    void importGltf();

signals:
    void importStarted();
    void importFinished();

};