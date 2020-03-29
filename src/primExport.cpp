#include "primExport.h"
#include "Console.h"
#include "GlacierFormats.h"

#include <QtConcurrent/qtconcurrentrun.h>

using namespace GlacierFormats;

void appendReferenceNode(const GlacierFormats::RuntimeId& id, const GlacierFormats::ResourceRepository* re, QStandardItem* node) {
    if (re->getResourceType(id) != "BORG" &&
        re->getResourceType(id) != "MATI" &&
        re->getResourceType(id) != "MATE" &&
        re->getResourceType(id) != "TEXD" &&
        re->getResourceType(id) != "TEXT" &&
        re->getResourceType(id) != "PRIM")
        return;

    std::string node_text = re->getResourceType(id);
    node_text += " ";
    node_text += id;
    node->setText(QString::fromStdString(node_text));

    auto ref = re->getResourceReferences(id);
    for (const auto& r : ref) {
        auto child = new QStandardItem();
        appendReferenceNode(r.id, re, child);
        node->appendRow(child);
    }
}

void PrimExportWidget::updateResourceDependencyTree(const QString& text) {
    if (text.isEmpty())
        return;

    RuntimeId prim_id = text.toStdString();
    auto repo = ResourceRepository::instance();

    QStandardItem* prim_item = new QStandardItem();
    appendReferenceNode(prim_id, repo, prim_item);

    QStandardItemModel* simPrimReferences = new QStandardItemModel(this);
    QStandardItem* siModelRoot = simPrimReferences->invisibleRootItem();
    siModelRoot->appendRow(prim_item);

    tvPrimReferences->setModel(simPrimReferences);
    tvPrimReferences->expandAll();
};

void PrimExportWidget::doExport() {
    auto console = Console::instance();

    RuntimeId id = cbPrimIds->currentText().toStdString();
    if (id == 0) {
        printError("Failed to export PRIM: No valid PRIM id");
        return;
    }

    std::filesystem::path export_dir = exportDirectory->path().toStdString();
    if (export_dir.empty()) {
        printError("Failed to export PRIM: No destination directory specified");
        return;
    }

    std::string msg = "Exporting " + std::string(id) + ".PRIM" +
        " to " + export_dir.generic_string() + std::string(id) + ".gltf\n";
    printStatus(msg);

    try {
        printStatus("Generating GlacierRenderAsset...");
        GlacierRenderAsset model(id);

        printStatus("Exporting Geometry...");
        Export::GLTFExporter{}(model, export_dir.generic_string());

        if (cbExportTextures->isChecked()) {
            printStatus("Exporting Textures...");
            Export::TGAExporter{}(model, export_dir.generic_string());
        }
    }
    catch (const std::exception& e) {
        printError(std::string(e.what()));
        return;
    }

    printStatus("\nPRIM exported successfully!\n");
}

void PrimExportWidget::exportModel() {
    emit exportStarted();
    auto future = QtConcurrent::run(this, &PrimExportWidget::doExport);
    while (!future.isFinished()) {
        qApp->processEvents();
        Sleep(33);
    }
    emit exportFinished();
}

PrimExportWidget::PrimExportWidget(QWidget* parent) : QGroupBox("PRIM Export", parent) {
    QGridLayout* exporterLayout = new QGridLayout(this);

    QBoxLayout* blExporterLine0 = new QBoxLayout(QBoxLayout::LeftToRight, this);
    auto prim_id_label = new QLabel("Prim Id:");
    prim_id_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    blExporterLine0->addWidget(prim_id_label, 0, 0);
    cbPrimIds = new QComboBox;
    cbPrimIds->addItem("");
    cbPrimIds->setEditable(true);
    cbPrimIds->setAutoCompletion(true);
    cbPrimIds->setMinimumWidth(300);
    cbPrimIds->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    reinterpret_cast<QListView*>(cbPrimIds->view())->setUniformItemSizes(true);
    reinterpret_cast<QListView*>(cbPrimIds->view())->setLayoutMode(QListView::Batched);

    auto prim_ids = GlacierFormats::ResourceRepository::instance()->getIdsByType("PRIM");
    std::sort(prim_ids.begin(), prim_ids.end());
    QStringList prim_id_list;
    prim_id_list.reserve(prim_ids.size());
    for (const auto& prim_id : prim_ids)
        prim_id_list.push_back(QString::fromStdString(prim_id));
    cbPrimIds->addItems(prim_id_list);

    connect(cbPrimIds, SIGNAL(editTextChanged(const QString&)), this, SLOT(updateResourceDependencyTree(const QString&)));
    blExporterLine0->addWidget(cbPrimIds, 0);

    exporterLayout->addLayout(blExporterLine0, 0, 0);

    tvPrimReferences = new QTreeView(this);
    tvPrimReferences->setHeaderHidden(true);
    exporterLayout->addWidget(tvPrimReferences, 1, 0);

    //Grid layout for check box options
    QGridLayout* glOptions = new QGridLayout(this);
    cbExportTextures = new QCheckBox(this);
    cbExportTextures->setText("Export Textures");
    cbExportTextures->setChecked(true);
    glOptions->addWidget(cbExportTextures, 0, 0);

    QComboBox* cbTextureFormat = new QComboBox(this);
    cbTextureFormat->addItem(".tga");
    cbTextureFormat->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    glOptions->addWidget(cbTextureFormat, 0, 1);

    QGroupBox* gbOptions = new QGroupBox("Options", this);
    gbOptions->setLayout(glOptions);

    exporterLayout->addWidget(gbOptions, 2, 0, 1, 2);

    exportDirectory = new PathBrowserWidget(PathBrowserType::OPEN_DIRECTORY, "Export directory:", "", this);
    exporterLayout->addWidget(exportDirectory, 3, 0, 1, 2);

    pbExportModel = new QPushButton("Export", this);
    exporterLayout->addWidget(pbExportModel, 4, 0, 1, 1);
    connect(pbExportModel, SIGNAL(clicked()), this, SLOT(exportModel()));

    setLayout(exporterLayout);
}