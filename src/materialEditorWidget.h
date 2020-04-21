#pragma once
#include <filesystem>
#include "GlacierFormats.h"
#include "primIdBrowserWidget.h"
#include "pathBrowser.h"

#include <QtWidgets>
#include <QVariant>
#include <QVector>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class MaterialPropertyItem {
public:
    explicit MaterialPropertyItem(GlacierFormats::Property* propertyNode, MaterialPropertyItem* parent = nullptr);
    explicit MaterialPropertyItem(const GlacierFormats::PropertyBinder* propertyNode, MaterialPropertyItem* parent = nullptr);
    explicit MaterialPropertyItem();
    ~MaterialPropertyItem();

    MaterialPropertyItem* child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    void appendChild(MaterialPropertyItem* child);
    MaterialPropertyItem* parent();
    int childNumber() const;
    bool setData(int column, const QVariant& value);
    bool isBinderItem() const;

private:
    QVector<MaterialPropertyItem*> childItems;
    QVector<QVariant> itemData;
    MaterialPropertyItem* parentItem;

    GlacierFormats::Property* propertyNode;

    bool setValueData(const QVariant& data);
    GlacierFormats::Property::PropertyVariantType getMaterialPropertyFromString(const std::string& str);
};

class MaterialPropertyModel : public QAbstractItemModel {
    Q_OBJECT

public:
    //The model keeps pointers to mati property nodes and updates them as the model is updated.
    MaterialPropertyModel(GlacierFormats::MATI* mati, QObject* parent = nullptr);
    ~MaterialPropertyModel();

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;

private:
    void setupModelData(MaterialPropertyItem* root, GlacierFormats::MATI* mati);
    MaterialPropertyItem* getItem(const QModelIndex& index) const;

    MaterialPropertyItem* rootItem;
};

class materialEditorWidget : public QWidget {
    Q_OBJECT

private:
    ResourceIdBrowserWidget* matiIdBrowser;

    std::unique_ptr<GlacierFormats::MATI> mati;
    std::unique_ptr<MaterialPropertyModel> model;
    QTreeView* view;

    PathBrowserWidget* exportPathWidget;

    void setViewParameters();

    void serializeToRpkg(const std::filesystem::path& path) const;
    void serializeToMati(const std::filesystem::path& path) const;

public:
    materialEditorWidget(QWidget* parent = nullptr);

public slots:
    void updateMaterial(const std::string& runtime_id);
    void serializeMaterial() const;

};