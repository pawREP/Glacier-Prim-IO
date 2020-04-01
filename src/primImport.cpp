#include "primImport.h"
#include "GlacierFormats.h"

#include <QtConcurrent/qtconcurrentrun.h>

#include <filesystem>
#include <regex>

using namespace GlacierFormats;

LabeledLineEdit::LabeledLineEdit(const QString& label, const QString& toolTip, const QString& placeholderText, QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->addWidget(new QLabel(label, this));

    line = new QLineEdit(this);
    line->setPlaceholderText(placeholderText);
    line->setToolTip(toolTip);
    layout->addWidget(line);

    setLayout(layout);
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

std::vector<uint64_t> DeletionList::deletionList() const {
    std::vector<uint64_t> ids;

    auto idList = text().toStdString();

    std::regex runtimeIdRegex("[0-9a-fA-F]{16}");

    std::smatch match;
    while(std::regex_search(idList, match, runtimeIdRegex)) {
            auto idString = std::string(match.str());
            ids.push_back(std::stoll(idString, nullptr, 16));
            idList = match.suffix();
    }

    return ids;
}

GltfImportOptions::GltfImportOptions(QWidget* parent) : QGroupBox("Options", parent) {
    auto layout = new QGridLayout(this);
    layout->setAlignment(Qt::AlignmentFlag::AlignLeft);
    setLayout(layout);

    cbImportTextures = new QCheckBox(this);
    cbImportTextures->setChecked(true);
    cbImportTextures->setText("Import Textures");
    cbImportTextures->setToolTip("Import all .tga textures found in the import folder");
    layout->addWidget(cbImportTextures, 0, 0);

    cbUseMaxLODRange = new QCheckBox(this);
    cbUseMaxLODRange->setText("Set max LOD range");
    cbUseMaxLODRange->setToolTip("Sets the LOD range of all mesh in the gltf to the max range");
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


GltfImportWidget::GltfImportWidget(QWidget* parent) : QGroupBox("PRIM Import", parent) {
    QGridLayout* importerLayout = new QGridLayout(this);

    //First line
    gltfBrowser = new PathBrowserWidget(PathBrowserType::OPEN_FILE, "GLTF File:", "GLTF (*.gltf)", this);
    connect(gltfBrowser, SIGNAL(pathChanged()), SLOT(gltfPathUpdated()));

    importerLayout->addWidget(gltfBrowser);

    teGltfInfo = new QTextEdit(this);
    teGltfInfo->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    teGltfInfo->setAttribute(Qt::WA_TransparentForMouseEvents);
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
        printError("Selected gltf file has invalid name. File name must be valid RuntimeId");
        return;
    }

    auto patchPath = repo->runtime_dir.generic_string();
    auto sourceArchiveName = repo->getSourceStreamName(id);
    patchPath += "/" + Util::incrementArchiveName(sourceArchiveName) + ".rpkg";

    patchFileBrowser->setPath(QString(patchPath.c_str()));
}

void GltfImportWidget::importGltf() {
    emit importStarted();
    auto future = QtConcurrent::run(this, &GltfImportWidget::doImport);
    while (!future.isFinished()) {
        qApp->processEvents();
        Sleep(33);
    }
    emit importFinished();
}

std::vector<uint64_t> getDeepTEXDReferenceIds(uint64_t prim_id) {
    auto repo = ResourceRepository::instance();

    std::vector<uint64_t> texd_ids;

    auto matis = repo->getResourceReferences(prim_id, "MATI");
    for (const auto& mati : matis) {
        auto texts = repo->getResourceReferences(mati.id, "TEXT");
        for (const auto& text : texts) {
            auto texds = repo->getResourceReferences(text.id, "TEXD");
            for (const auto& texd : texds)
                texd_ids.push_back(texd.id);
        }
    }

    return texd_ids;
}

std::vector<std::unique_ptr<TEXD>> importTextures(uint64_t prim_id, const std::filesystem::path& texture_folder) {
    auto repo = ResourceRepository::instance();

    std::vector<std::unique_ptr<TEXD>> textures;

    auto texd_ids = getDeepTEXDReferenceIds(prim_id);
    for (const auto& texd_id : texd_ids) {
        std::filesystem::path texture_path = texture_folder / (static_cast<std::string>(RuntimeId(texd_id)) + ".tga");
        if (texture_path.empty() ||
            !std::filesystem::exists(texture_path) ||
            !std::filesystem::is_regular_file(texture_path)
            )
            continue;

        try {
            textures.push_back(std::move(TEXD::loadFromTGAFile(texture_path)));
            textures.back()->id = RuntimeId(texture_path.stem().generic_string());
        }
        catch (const std::exception& e) {
            printError(std::string("TGA Texture load failed: ") + e.what());
        }
    }
    return textures;
}

void GltfImportWidget::doImport() {
    auto repo = ResourceRepository::instance();

    auto gltfFilePath = std::filesystem::path(gltfBrowser->path().toStdString());
    if (!isValidOpenFilePath(gltfFilePath)) {
        printError("Gltf file path invalid");
        return;
    }

    auto patchFilePath = std::filesystem::path(patchFileBrowser->path().toStdString());
    if (!isValidSaveFilePath(patchFilePath)) {
        printError("Patch file path invalid");
        return;
    }

    RuntimeId prim_id = gltfFilePath.stem().generic_string();
    if (!repo->contains(prim_id)) {
        printError("Gltf file name invalid. File name must be valid RuntimeId");
        return;
    }

    printStatus("GLTF Import:\n    " + gltfFilePath.generic_string() + "\n        ->\n    " + patchFilePath.generic_string() + "\n");

    auto borgReferences = repo->getResourceReferences(prim_id, "BORG");
    GLACIER_ASSERT_TRUE(borgReferences.size() <= 1);

    printStatus("Building GLTFAsset...");
    std::unique_ptr<GLTFAsset> asset = nullptr;
    if (borgReferences.size()) {//weighted/linked PRIM
        auto borg = repo->getResource<BORG>(borgReferences.front().id);
        GLACIER_ASSERT_TRUE(borg);
        auto bone_mapping = borg->getNameToBoneIndexMap();
        try {
            asset = std::make_unique<GLTFAsset>(gltfFilePath, &bone_mapping);
        }
        catch (const std::exception& e) {
            printError(e.what());
            return;
        }
    }
    else {//standard PRIM
        try {
            asset = std::make_unique<GLTFAsset>(gltfFilePath);
            GLACIER_ASSERT_TRUE(asset);
        }
        catch (const std::exception& e) {
            printError(e.what());
            return;
        }
    }

    printStatus("Parsing original PRIM...");
    std::unique_ptr<PRIM> originalPrim = nullptr;
    try {
        originalPrim = repo->getResource<GlacierFormats::PRIM>(prim_id);
        GLACIER_ASSERT_TRUE(originalPrim);
    }
    catch (const std::exception& e) {
        printError(e.what());
        return;
    }

    printStatus("Building new PRIM from GLTFAsset...");
    std::function<void(ZRenderPrimitiveBuilder&, const std::string&)> build_modifier =
        [repo, prim_id, &originalPrim](GlacierFormats::ZRenderPrimitiveBuilder& builder, const std::string& submesh_name) -> void {
        for (auto& primitive : originalPrim->primitives) {
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

    std::unique_ptr<PRIM> prim = nullptr;
    try {
        prim = std::make_unique<PRIM>(asset->meshes(), prim_id, &build_modifier);
    }
    catch (const std::exception& e) {
        printError(e.what());
        return;
    }

    //Post-process primitives
    if (borgReferences.size())//weighted/linked PRIM
        prim->manifest.rig_index = 0;
    else
        prim->manifest.rig_index = -1;
    prim->manifest.properties = originalPrim->manifest.properties;

    for (auto& primitive : prim->primitives) {
        if (options->useMaxLODRange())
            primitive->remnant.lod_mask = 0xFF;
        if (options->useCustomMaterialId())
            primitive->remnant.material_id = options->materialId();
    }

    printStatus("Serializing PRIM to patch file...");
    GlacierFormats::RPKG rpkg{};

    auto prim_data = prim->serializeToBuffer();
    auto refs = repo->getResourceReferences(prim_id);
    rpkg.insertFile(prim_id, "PRIM", prim_data, &refs);

    printStatus("Importing and serializing textures...");
    if (options->importTextures()) {
        auto textures = importTextures(prim_id, gltfFilePath.parent_path());
        for (const auto& texture : textures) {
            auto texd_data = texture->serializeToBuffer();
            rpkg.insertFile(texture->id, "TEXD", texd_data);
        }
    }

    //Deletion list
    for (const auto& id : deletionList->deletionList())
        rpkg.deletion_list.push_back(id);

    printStatus("Writing patch file...");
    try {
        rpkg.write(patchFilePath);
    }
    catch (const std::exception& e) {
        printError(e.what());
        return;
    }
    printStatus("Imported gltf sucesfully!\n");
}