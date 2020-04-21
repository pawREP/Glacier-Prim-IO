#include "materialEditorWidget.h"
#include "Console.h"
#include <regex>
#include <sstream>
#include <QTreeView>

using namespace GlacierFormats;

constexpr char STRING_STR[] = "String";
constexpr char INT_STR[] = "Int";
constexpr char FLOAT_STR[] = "Float";
constexpr char VECTOR_INT_STR[] = "Vector<Int>";
constexpr char VECTOR_FLOAT_STR[] = "Vector<Float>";

constexpr int COLUMN_KEY = 0;
constexpr int COLUMN_DESC = 1;
constexpr int COLUMN_TYPE = 2;
constexpr int COLUMN_VALUE = 3;

const std::unordered_map<std::string, std::string> keyDescriptions{
    {"INST", "Instance"},
    {"TAGS", "Tags"},
    {"NAME", "Name"},
    {"BIND", "Binder"},
    {"RSTA", "Render State"},
    {"FLTV", "Float Value"},
    {"TEXT", "Texture"},
    {"COLO", "Color"},
    {"ENAB", "Enabled"},
    {"CULL", "Culling Mode"},
    {"SSVR", "Subsurface Red"},
    {"SSVG", "Subsurface Green"},
    {"SSVB", "Subsurface Blue"},
    {"SSBW", "Subsurface Value"},
    {"TXID", "Texture Id"},
    {"TILU", "Tiling U"},
    {"TILV", "Tiling V"},
    {"VALU", "Value"},
    {"TYPE", "Type"},
    {"ZBIA", "Z Bias"},
    {"ZOFF", "Z Offset"},
    {"BMOD", "Blend Mode"},
    {"OPAC", "Opacity"},
    {"DBDE", "Decal Blend Diffuse"},
    {"DBNE", "Decal Blend Normal"},
    {"DBSE", "Decal Blend Specular"},
    {"DBRE", "Decal Blend Roughness"},
    {"DBEE", "Decal Blend Emission"},
    {"BENA", "Blend Enabled"},
    {"FENA", "Fog Enabled"},
    {"ATST", "Alpha Test Enabled"},
    {"AREF", "Alpha Reference"},
};

const auto FLT_REGEX = std::regex(R"(-?[0-9]{1,}\.?[0-9]{0,})");
const auto INT_REGEX = std::regex(R"(-?\d+)");

std::vector<std::string> regexFindAll(const std::string& str, const std::regex& regex) {
    std::vector<std::string> matches;

    std::smatch match;
    std::string::const_iterator str_it(str.cbegin());
    while (regex_search(str_it, str.cend(), match, regex)) {
        matches.push_back(match[0]);
        str_it = match.suffix().first;
    }

    return matches;
}

template<typename Variant>
QString stringify(const Variant& variant) {
    QString str;

    std::visit([&str](const auto& val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, std::string>) {
            str = QString::fromStdString(val);
        }

        if constexpr (std::is_same_v<T, int>) {
            std::stringstream ss;
            ss << val;
            str = QString::fromStdString(ss.str());
        }

        if constexpr (std::is_same_v<T, float>) {
            std::stringstream ss;
            ss.precision(3);
            ss << std::fixed << val;
            str = QString::fromStdString(ss.str());
        }

        if constexpr (std::is_same_v<T, std::vector<int>>) {
            std::stringstream ss;
            ss << "[";
            for (int i = 0; i < val.size() - 1; ++i)
                ss << val.at(i) << ", ";
            ss << val.back() << "]";
            str = QString::fromStdString(ss.str());
        }

        if constexpr (std::is_same_v<T, std::vector<float>>) {
            std::stringstream ss;
            ss.precision(3);
            ss << "[";
            for (int i = 0; i < val.size() - 1; ++i)
                ss << std::fixed << val.at(i) << ", ";
            ss << val.back() << "]";
            str = QString::fromStdString(ss.str());
        }
    }, variant);

    return str;
}

template<typename Variant>
QString variantToData(const Variant& val) {
    return stringify(val->data());
}

MaterialPropertyItem::MaterialPropertyItem(GlacierFormats::Property* propertyNode, MaterialPropertyItem* parent) : 
    parentItem(parent),
    propertyNode(propertyNode) {
    itemData.resize(4);

    if (!propertyNode)
        return;

    const std::string key = propertyNode->name();
    setData(COLUMN_KEY, key.c_str());
    if (keyDescriptions.find(key) != keyDescriptions.end())
        setData(COLUMN_DESC, keyDescriptions.at(key).c_str());
    
    //TODO factor the type string append into fn
    if (propertyNode->is<std::string>())
        setData(COLUMN_TYPE, STRING_STR);
    else if (propertyNode->is<int>())
        setData(COLUMN_TYPE, INT_STR);
    else if (propertyNode->is<float>())
        setData(COLUMN_TYPE, FLOAT_STR);
    else if (propertyNode->is<std::vector<int>>())
        setData(COLUMN_TYPE, VECTOR_INT_STR);
    else if (propertyNode->is<std::vector<float>>())
        setData(COLUMN_TYPE, VECTOR_FLOAT_STR);

    setData(COLUMN_VALUE, stringify(propertyNode->data()));
}

MaterialPropertyItem::MaterialPropertyItem(const GlacierFormats::PropertyBinder* binder, MaterialPropertyItem* parent) :
    parentItem(parent),
    propertyNode(nullptr) {
    itemData.resize(4);

    const std::string binder_name = binder->name();

    setData(COLUMN_KEY, binder_name.c_str());
    if (keyDescriptions.find(binder_name) != keyDescriptions.end())
        setData(COLUMN_DESC, keyDescriptions.at(binder_name).c_str());
}

MaterialPropertyItem::MaterialPropertyItem() :
    parentItem(nullptr),
    propertyNode(nullptr) {
    itemData.resize(4);
}

MaterialPropertyItem::~MaterialPropertyItem() {
    qDeleteAll(childItems);
}

MaterialPropertyItem* MaterialPropertyItem::child(int number) {
    if (number < 0 || number >= childItems.size())
        return nullptr;
    return childItems.at(number);
}

int MaterialPropertyItem::childCount() const {
    return childItems.count();
}

int MaterialPropertyItem::childNumber() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<MaterialPropertyItem*>(this));
    return 0;
}

int MaterialPropertyItem::columnCount() const {
    return itemData.count();
}

QVariant MaterialPropertyItem::data(int column) const {
    if (column < 0 || column >= itemData.size())
        return QVariant();
    return itemData.at(column);
}

void MaterialPropertyItem::appendChild(MaterialPropertyItem* child) {
    childItems.push_back(child);
}

MaterialPropertyItem* MaterialPropertyItem::parent() {
    return parentItem;
}

bool MaterialPropertyItem::setData(int column, const QVariant& value) {
    if (column < 0 || column >= itemData.size())
        return false;


    //Update mati node and regularize value string 
    if (column == COLUMN_VALUE) {
        const auto& oldValueData = itemData[column];
        if (!setValueData(value))
            itemData[column] = oldValueData;
    }
    else {
        itemData[column] = value;
    }

    return true;
}

bool MaterialPropertyItem::isBinderItem() const {
    return propertyNode == nullptr;
}

//Returns property variant based on string. Return std::monostate if input is invalid.
Property::PropertyVariantType MaterialPropertyItem::getMaterialPropertyFromString(const std::string& str) {
    Property::PropertyVariantType variant;

    std::visit([&variant, &str](const auto& var) {
        using T = std::decay_t<decltype(var)>;
        if constexpr (std::is_same_v<T, std::string>) {
            variant = str;
        }
        if constexpr (std::is_same_v<T, int>) {
            const auto intStrs = regexFindAll(str, INT_REGEX);
            if (intStrs.size() == 1)
                variant = std::stoi(intStrs[0]);
        }
        if constexpr (std::is_same_v<T, float>) {
            const auto floatStrs = regexFindAll(str, FLT_REGEX);
            if (floatStrs.size() == 1)
                variant = std::stof(floatStrs[0]);
        }
        if constexpr (std::is_same_v<T, std::vector<int>>) {
            std::vector<int> ints;
            const auto intStrs = regexFindAll(str, INT_REGEX);
            for (const auto& i : intStrs)
                ints.push_back(std::stoi(i));
            if(ints.size() == var.size())
                variant = ints;
        }
        if constexpr (std::is_same_v<T, std::vector<float>>) {
            std::vector<float> floats;
            const auto floatStrs = regexFindAll(str, FLT_REGEX);
            for (const auto& i : floatStrs)
                floats.push_back(std::stof(i));
            variant = floats;
        }
    }, propertyNode->data());

    return variant;
}

//Attempts to set data of value column based on used input.
bool MaterialPropertyItem::setValueData(const QVariant& data) {
    if (!propertyNode) {
        itemData[COLUMN_VALUE] = data;
        return true;
    }

    assert(data.canConvert<QString>());

    const auto& data_str = data.toString().toStdString();
    const auto& variant = getMaterialPropertyFromString(data_str);

    if (!variant.index())
        return false;

    itemData[COLUMN_VALUE] = stringify(variant);
    propertyNode->setData(variant);

    return true;
}

MaterialPropertyModel::MaterialPropertyModel(GlacierFormats::MATI* mati, QObject* parent) : QAbstractItemModel(parent) {
    rootItem = new MaterialPropertyItem();
    rootItem->setData(COLUMN_KEY, "Key");
    rootItem->setData(COLUMN_DESC, "Desc");
    rootItem->setData(COLUMN_TYPE, "Type");
    rootItem->setData(COLUMN_VALUE, "Value");
    setupModelData(rootItem, mati);
}

MaterialPropertyModel::~MaterialPropertyModel() {
    delete rootItem;
}

int MaterialPropertyModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return rootItem->columnCount();
}

QVariant MaterialPropertyModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::ForegroundRole && getItem(index)->isBinderItem()) {
        return QVariant(QColor(42, 130, 218));//TODO: unify color scheme vars.
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    MaterialPropertyItem* item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags MaterialPropertyModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    if(index.column() == 3)
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    return QAbstractItemModel::flags(index);
}

MaterialPropertyItem* MaterialPropertyModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        MaterialPropertyItem* item = static_cast<MaterialPropertyItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

QVariant MaterialPropertyModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex MaterialPropertyModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    MaterialPropertyItem* parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    MaterialPropertyItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex MaterialPropertyModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    MaterialPropertyItem* childItem = getItem(index);
    MaterialPropertyItem* parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int MaterialPropertyModel::rowCount(const QModelIndex& parent) const {
    const MaterialPropertyItem* parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}



bool MaterialPropertyModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role != Qt::EditRole)
        return false;

    MaterialPropertyItem* item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });

    return result;
}

bool MaterialPropertyModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    const bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void processNode(MaterialPropertyItem* parent, GlacierFormats::PropertyNode& node) {
    if (node.is<PropertyBinder>()) {
        auto& binder = node.get<PropertyBinder>();
        MaterialPropertyItem* child = new MaterialPropertyItem(&binder, parent);
        parent->appendChild(child);

        for (auto& binder_child : binder)
            processNode(child, binder_child);
    }
    else if (node.is<Property>()) {
        auto& prop = node.get<Property>();
        MaterialPropertyItem* child = new MaterialPropertyItem(&prop, parent);
        parent->appendChild(child);
    }
}

void MaterialPropertyModel::setupModelData(MaterialPropertyItem* root, GlacierFormats::MATI* mati) {
    processNode(root, mati->properties->root());
}

void materialEditorWidget::updateMaterial(const std::string& runtime_id) {
    auto repo = ResourceRepository::instance();
    if (repo->getResourceType(runtime_id) != "MATI")
        return;
    
    view->setModel(nullptr);
    model.reset(nullptr);

    mati = repo->getResource<MATI>(runtime_id);
    model = std::make_unique<MaterialPropertyModel>(mati.get());
    view->setModel(model.get());
    setViewParameters();
}

void materialEditorWidget::setViewParameters() {
    view->setColumnWidth(0, 150);
    view->setColumnWidth(1, 150);
    view->setColumnWidth(2, 100);
    view->expandAll();
}

void materialEditorWidget::serializeToRpkg(const std::filesystem::path& path) const {
    RPKG rpkg{};
    rpkg.insertFile(mati->id, "MATI", mati->serializeToBuffer());
    rpkg.write(path);
}

void materialEditorWidget::serializeToMati(const std::filesystem::path& path) const {
    mati->serializeToFile(path);
}

void materialEditorWidget::serializeMaterial() const {
    if (!mati) {
        printError("No MATI loaded");
        return;
    }

    const auto& path = std::filesystem::path(exportPathWidget->path().toStdString());
    if (path.empty() || !path.has_filename() || !path.has_extension()) {
        printError("Invalid export path");
        return;
    }

    const auto& extension = path.extension();
    if (extension == ".rpkg") {
        serializeToRpkg(path);
    }
    else if (extension == ".mati") {
        serializeToMati(path);
    }
    else {
        printError("Invalid export path");
        return;
    }

    printStatus("\nMATI (0x" + mati->name() + ") exported successfully!");
}

materialEditorWidget::materialEditorWidget(QWidget* parent) : QWidget(parent) {
	auto layout = new QVBoxLayout(this);

	matiIdBrowser = new ResourceIdBrowserWidget("MATI", this);
    connect(matiIdBrowser, SIGNAL(idChanged(const std::string&)), this, SLOT(updateMaterial(const std::string&)));
	layout->addWidget(matiIdBrowser);
	
    view = new QTreeView(this);
    layout->addWidget(view);

    exportPathWidget = new PathBrowserWidget(PathBrowserType::SAVE_FILE, "Export Path:", "Patch (*.rpkg);; Material Instance (*.MATI)", this);
    layout->addWidget(exportPathWidget);

    QPushButton* exportButton = new QPushButton("Export", this);
    connect(exportButton, SIGNAL(clicked()), this, SLOT(serializeMaterial()));
    layout->addWidget(exportButton);


	setLayout(layout);
}