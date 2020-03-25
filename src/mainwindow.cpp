#include <QtWidgets>
#include <QtConcurrent/qtconcurrentrun.h>

#include "GlacierFormats.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <regex>

using namespace GlacierFormats;

MainWindow::~MainWindow() {

}

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle(tr("Glacier Prim I/O tool"));

    QGridLayout* glMainLayout = new QGridLayout(this);

    runtimeDirectory = new PathBrowserWidget(PathBrowserType::OPEN_DIRECTORY, "Runtime directory:", "", this);
    glMainLayout->addWidget(runtimeDirectory, 0, 0, 1, 3);

    QGroupBox* gbExport = new QGroupBox("PRIM Export", this);
    primExportWidget = new PrimExportWidget(this);
    gbExport->setLayout(primExportWidget->layout());
    glMainLayout->addWidget(gbExport, 1, 0, 1, 1);

    QGroupBox* gbImport = new QGroupBox("PRIM Import", this);
    gltfImportWidget = new GltfImportWidget(this);
    gbImport->setLayout(gltfImportWidget->layout());
    glMainLayout->addWidget(gbImport, 1, 2, 1, 1);

    console = new ConsoleWidget(this);
    Console::instance().setDestinationWidget(console);
    Console::instance().print("Glacier PRIM I/O v1.00 by B3\n");
    glMainLayout->addWidget(console, 2, 0, 1, 3);

    glMainLayout->addWidget(new FooterBar(this), 3, 0, 1, 3);

    initilizeWindowsComponents();
}

GltfImportWidget::GltfImportWidget(QWidget*  parent) : QWidget(parent) {
    QGridLayout* importerLayout = new QGridLayout(this);

    //First line
    gltfBrowser = new PathBrowserWidget(PathBrowserType::OPEN_FILE, "GLTF File:", "GLTF (*.gltf)", this);
    connect(gltfBrowser, SIGNAL(pathChanged()), SLOT(gltfPathUpdated()));

    importerLayout->addWidget(gltfBrowser);

    //GLTF meta data
    teGltfInfo = new QTextEdit(this);
    teGltfInfo->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    teGltfInfo->setAttribute(Qt::WA_TransparentForMouseEvents);
    teGltfInfo->setText(
        "GLTF:\t001481248949819.gltf\nTextures:\t3\nMaterials:\t1"
    );
    teGltfInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    importerLayout->addWidget(teGltfInfo);

    options = new GltfImportOptions(this);
    importerLayout->addWidget(options);

    deletionList = new DeletionList(this);
    importerLayout->addWidget(deletionList);

    patchFileBrowser = new PathBrowserWidget(PathBrowserType::SAVE_FILE, "Patch File:", "RPKG (*.rpkg)", this);
    importerLayout->addWidget(patchFileBrowser);

    pbImport = new QPushButton("Import", this);
    connect(pbImport, SIGNAL(clicked()), SLOT(importGltf()));
    importerLayout->addWidget(pbImport);
}

void GltfImportWidget::gltfPathUpdated() {
    auto gltfPath = gltfBrowser->path();
    //Generate appropriate patch file name; 
    auto repo = ResourceRepository::instance();

    RuntimeId id = std::filesystem::path(gltfPath.toStdString()).stem().generic_string();
    if (!repo->contains(id)) {
        Console::instance().print("Selected gltf file has invalid name. File name must be valid RuntimeId", Qt::red);
        return;
    }

    auto patchPath = repo->runtime_dir.generic_string();
    auto sourceArchiveName = repo->getSourceStreamName(id);
    patchPath += "/" + GlacierFormats::Util::incrementArchiveName(sourceArchiveName) + ".rpkg";

    patchFileBrowser->setPath(QString(patchPath.c_str()));
}

void GltfImportWidget::importGltf() {
    auto future = QtConcurrent::run(this, &GltfImportWidget::doImport);
    while (!future.isFinished()) {
        qApp->processEvents();
        Sleep(33);
    }
}

void GltfImportWidget::doImport() {
    auto repo = ResourceRepository::instance();

    auto destinationPath = std::filesystem::path(patchFileBrowser->path().toStdString());
    auto gltfPath = std::filesystem::path(gltfBrowser->path().toStdString());

    Console::instance().print(
        "GLTF Import:\n    " + 
        gltfPath.generic_string() + "\n"
        "        ->\n    " + 
        destinationPath.generic_string() + "\n"
    );
    
    RuntimeId id = gltfPath.stem().generic_string();
    if (!repo->contains(id)) {
        Console::instance().print("Selected gltf file has invalid name. File name must be valid RuntimeId", Qt::red);
        return;
    }

    auto borgReferences = repo->getResourceReferences(id, "BORG");
    GLACIER_ASSERT_TRUE(borgReferences.size() <= 1);

    Console::instance().print("Building GLTFAsset...");
    std::unique_ptr<GLTFAsset> asset = nullptr;
    if (borgReferences.size()) {//weighted/linked PRIM
        auto borg = repo->getResource<BORG>(borgReferences.front().id);
        GLACIER_ASSERT_TRUE(borg);
        auto bone_mapping = borg->getNameToBoneIndexMap();
        try {
            asset = std::make_unique<GLTFAsset>(gltfPath, &bone_mapping);
        }
        catch (const std::exception & e) {
            Console::instance().print("GLTFAsset construction failed.", Qt::red);
            Console::instance().print(e.what(), Qt::red);
            return;
        }
    }
    else {//standard PRIM
        try{
            asset = std::make_unique<GLTFAsset>(gltfPath);
        }
        catch (const std::exception & e) {
            Console::instance().print("GLTFAsset construction failed.", Qt::red);
            Console::instance().print(e.what(), Qt::red);
            return;
        }
    }
    Console::instance().print("...OK\n");

    Console::instance().print("Parsing original PRIM...");
    auto original_prim = repo->getResource<GlacierFormats::PRIM>(id);
    GLACIER_ASSERT_TRUE(original_prim);
    std::function<void(ZRenderPrimitiveBuilder&, const std::string&)> build_modifier =
        [repo, id, &original_prim = std::as_const(original_prim)](GlacierFormats::ZRenderPrimitiveBuilder& builder, const std::string& submesh_name) -> void {
        for (auto& primitive : original_prim->primitives) {
            auto primitive_name = primitive->name();
            if (primitive_name == submesh_name) {
                builder.setMaterialId(primitive->materialId());
                builder.setLodMask(primitive->remnant.lod_mask);
                builder.setPropertyFlags(primitive->remnant.submesh_properties);
                builder.setColor1(primitive->remnant.submesh_color1);
                builder.setMeshSubtype(primitive->remnant.mesh_subtype);
            }
        }
    };
    Console::instance().print("...OK\n");

    Console::instance().print("Building new PRIM from GLTFAsset...");
    std::unique_ptr<PRIM> prim = nullptr;
    try{
        prim = std::make_unique<PRIM>(asset->meshes(), id, &build_modifier);
    }
    catch (const std::exception & e) {
        Console::instance().print("PRIM construction from GLTFAsset meshes failed.", Qt::red);
        Console::instance().print(e.what(), Qt::red);
        return;
    }
    Console::instance().print("...OK\n");

    if (borgReferences.size())//weighted/linked PRIM
        prim->manifest.rig_index = 0;
    else
        prim->manifest.rig_index = -1;
    prim->manifest.properties = original_prim->manifest.properties;

    for (auto& primitive : prim->primitives) {
        if(options->useMaxLODRange())
            primitive->remnant.lod_mask = 0xFF;
        if (options->useCustomMaterialId())
            primitive->remnant.material_id = options->materialId();
    }

    Console::instance().print("Serializing PRIM to patch file...");
    GlacierFormats::RPKG rpkg{};

    std::unique_ptr<char[]> prim_data = nullptr;
    int64_t prim_data_size = prim->serializeToBuffer(prim_data);
    auto refs = repo->getResourceReferences(id);
    rpkg.insertFile(id, "PRIM", prim_data.get(), prim_data_size, &refs);

    //Insert TEXTURES

    //Deletion list
    rpkg.deletion_list = deletionList->deletionList();

    rpkg.write(destinationPath);
    Console::instance().print("...OK\n");

    Console::instance().print("Imported gltf sucesfully!\n");
}

QLayout* MainWindow::createDebugOutConsoleLayout() {
    return nullptr;
}

void MainWindow::initilizeWindowsComponents() {
    runtimeDirectory->setPath(QString::fromStdString(GlacierFormats::ResourceRepository::runtime_dir.generic_string()));
    runtimeDirectory->setEnabled(false);

    //Export Window
    auto prim_ids = GlacierFormats::ResourceRepository::instance()->getIdsByType("PRIM");
    std::sort(prim_ids.begin(), prim_ids.end());
    QStringList prim_id_list;
    prim_id_list.reserve(prim_ids.size());
    for (const auto& prim_id : prim_ids)
        prim_id_list.push_back(QString::fromStdString(prim_id));

    primExportWidget->cbPrimIds->addItems(prim_id_list);
}

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
        console.print("Failed to export PRIM: No valid PRIM id", Qt::red);
        return;
    }

    std::filesystem::path export_dir = exportDirectory->path().toStdString();
    if (export_dir.empty()) {
        console.print("Failed to export PRIM: No destination directory specified", Qt::red);
        return;
    }

    std::string msg = "Exporting " + std::string(id) + ".PRIM" +
        " to " + export_dir.generic_string() + std::string(id) + ".gltf\n";
    console.print(msg);

    try {
        console.print("Generating GlacierRenderAsset...");
        GlacierRenderAsset model(id);
        console.print("    ...OK");

        console.print("Exporting Geometry...");
        Export::GLTFExporter{}(model, export_dir.generic_string());
        console.print("    ...OK");

        if (cbExportTextures->isChecked()) {
            console.print("Exporting Textures...");
            Export::TGAExporter{}(model, export_dir.generic_string());
            console.print("    ...OK");
        }
    }
    catch (const std::exception & e) {
        console.print("Fatal Exception:", Qt::red);
        console.print("\t" + std::string(e.what()) + "\n", Qt::red);
        return;
    }

    console.print("\nPRIM exported successfully!\n");
}

void PrimExportWidget::exportModel() {
    auto future = QtConcurrent::run(this, &PrimExportWidget::doExport);
    while (!future.isFinished()) {
        qApp->processEvents();
        Sleep(33);
    }
}

ConsoleWidget::ConsoleWidget(QWidget* parent) : QWidget(parent) {
    text = new QTextEdit(this);

    auto boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(text);
    this->setLayout(boxLayout);
}

ConsoleWidget::~ConsoleWidget() {

}

void ConsoleWidget::print(const char* message, const QColor& color) {
    print(QString(message), color);
}

void ConsoleWidget::print(const std::string& message, const QColor& color) {
    print(QString::fromStdString(message), color);
}

void ConsoleWidget::print(const QString& message, const QColor& color) {
    auto cursor = text->textCursor();
    cursor.setPosition(text->document()->characterCount() - 1);
    text->setTextCursor(cursor);
    text->setTextColor(color);
    text->append(message);
}

PrimExportWidget::PrimExportWidget(QWidget* parent) : QWidget(parent) {
    QGridLayout* exporterLayout = new QGridLayout(this);

    //First line

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

    connect(cbPrimIds, SIGNAL(editTextChanged(const QString&)), this, SLOT(updateResourceDependencyTree(const QString&)));
    blExporterLine0->addWidget(cbPrimIds, 0);

    exporterLayout->addLayout(blExporterLine0, 0, 0);

    //ThirdLine
    tvPrimReferences = new QTreeView(this);
    tvPrimReferences->setHeaderHidden(true);
    exporterLayout->addWidget(tvPrimReferences, 1, 0);

    //Grid layout for check box options
    QGridLayout* glOptions = new QGridLayout(this);
    cbExportTextures = new QCheckBox(this);
    cbExportTextures->setText("Export Textures");
    cbExportTextures->setChecked(true);
    glOptions->addWidget(cbExportTextures,0,0);

    QComboBox* cbTextureFormat = new QComboBox(this);
    cbTextureFormat->addItem(".tga");
    cbTextureFormat->addItem(".png");
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

PrimExportWidget::~PrimExportWidget() {
}

GltfImportWidget::~GltfImportWidget() {
}

Console& Console::instance() {
    static Console inst;
    return inst;
}

void Console::setDestinationWidget(ConsoleWidget* widget) {
    console_widget = widget;
}

GltfImportOptions::GltfImportOptions(QWidget* parent) : QGroupBox("Options", parent) {
    auto layout = new QGridLayout(this);
    layout->setAlignment(Qt::AlignmentFlag::AlignLeft);
    setLayout(layout);

    cbImportTextures = new QCheckBox(this);
    cbImportTextures->setChecked(true);
    cbImportTextures->setText("Import Textures");
    cbImportTextures->setToolTip("Import all textures found in the gltf file");
    layout->addWidget(cbImportTextures, 0, 0);

    cbUseMaxLODRange = new QCheckBox(this);
    cbUseMaxLODRange->setText("Set max LOD range");
    cbUseMaxLODRange->setToolTip("Sets the LOD range of all mesh in the gltf to the max range. This is useful when you deleted the original LODs.");
    layout->addWidget(cbUseMaxLODRange, 1, 0);

    cbUseCustomMaterialId = new QCheckBox(this);
    cbUseCustomMaterialId->setText("Override material Ids");
    cbUseCustomMaterialId->setToolTip("Sets the material id of all meshes to the given id");
    connect(cbUseCustomMaterialId, SIGNAL(stateChanged(int)), SLOT(materialIdOverrideChecked(int)));
    layout->addWidget(cbUseCustomMaterialId, 2, 0);

    sbMaterialId = new QSpinBox(this);
    sbMaterialId->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sbMaterialId->setEnabled(false);
    layout->addWidget(sbMaterialId, 2, 1);
}

void GltfImportOptions::materialIdOverrideChecked(int state) {
    switch (state) {
    case Qt::Checked:
        sbMaterialId->setEnabled(true);
        break;
    case Qt::Unchecked:
        sbMaterialId->setEnabled(false);
        break;
    default:
        throw;
    }
}

GltfImportOptions::~GltfImportOptions()
{
}

bool GltfImportOptions::importTextures() {
    return cbImportTextures->checkState() == Qt::Checked;
}

bool GltfImportOptions::useMaxLODRange() {
    return cbUseMaxLODRange->checkState() == Qt::Checked;
}

bool GltfImportOptions::useCustomMaterialId() {
    return cbUseCustomMaterialId->checkState() == Qt::Checked;
}

int GltfImportOptions::materialId() {
    return sbMaterialId->value();
}

PathBrowserWidget::PathBrowserWidget(PathBrowserType type, const QString& label, const QString& filter, QWidget* parent) : QWidget(parent) {
    this->filter = filter;
    this->type = type;

    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->addWidget(new QLabel(label, this));

    lePath = new QLineEdit(this);
    lePath->setText("");
    lePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(lePath);

    pbBrowse = new QPushButton("Browse...", this);
    connect(pbBrowse, SIGNAL(clicked()), SLOT(browse()));
    layout->addWidget(pbBrowse);

    setLayout(layout);
}

PathBrowserWidget::~PathBrowserWidget() {

}

QString PathBrowserWidget::path() const {
    return lePath->text();
}

void PathBrowserWidget::setPath(const QString& path) {
    lePath->setText(path);
}

void PathBrowserWidget::browse() {
    QString path;
    switch (type) {
    case PathBrowserType::SAVE_FILE:
        path = QFileDialog::getSaveFileName(this, "Browse Save File", QDir::currentPath(), filter);
        break;
    case PathBrowserType::OPEN_FILE:
        path = QFileDialog::getOpenFileName(this, "Browse File", QDir::currentPath(), filter);
        break;
    case PathBrowserType::OPEN_DIRECTORY:
        path = QFileDialog::getExistingDirectory(this, "Browse Directory", QDir::currentPath());
        break;
    default:
        throw;
    }
    lePath->setText(path);
    emit pathChanged();
}

FooterBar::FooterBar(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);

    auto margins = layout->contentsMargins();
    margins.setTop(0);
    margins.setBottom(0);
    layout->setContentsMargins(margins);

    auto creditLabel = new QLabel("PRIM I/O Tool by B3.", this);
    creditLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(creditLabel);
    
    auto exitButton = new QPushButton("Exit", this);
    connect(exitButton, SIGNAL(clicked()), SLOT(exitClicked()));
    layout->addWidget(exitButton);

    setLayout(layout);
}

FooterBar::~FooterBar() {

}

void FooterBar::exitClicked() {
    exit(0);
}

LabeledLineEdit::LabeledLineEdit(const QString& label, const QString& toolTip, const QString& placeholderText, QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    
    layout->addWidget(new QLabel(label, this));

    line = new QLineEdit(this);
    line->setPlaceholderText(placeholderText);
    line->setToolTip(toolTip);
    layout->addWidget(line);

    setLayout(layout);
}

LabeledLineEdit::~LabeledLineEdit() {

}

QString LabeledLineEdit::text() const {
    return line->text();
}

DeletionList::DeletionList(QWidget* parent) :
    LabeledLineEdit("Disabled Resources:",
        "List of runtime ids that should be marked as disabled by the generated patch.",
        "001481248949819, 00d4a4a176a10980, ...",
        parent) {
    
}

DeletionList::~DeletionList() {

}

std::vector<uint64_t> DeletionList::deletionList() const {
    std::vector<uint64_t> ids;

    auto idList = text().toStdString();
    
    std::regex runtimeIdRegex("[0-9a-fA-F]{16}");

    std::smatch matches;
    if (std::regex_match(idList, matches, runtimeIdRegex)) {
        for (int i = 1; i < matches.size(); ++i) {
            auto idString = std::string(matches[i].str());
            idList.push_back(std::stoll(idString, nullptr, 16));
        }
    }

    return ids;
}
