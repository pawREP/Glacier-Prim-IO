#include "primIdBrowserWidget.h"
#include "GlacierFormats.h"

using namespace GlacierFormats;

ResourceIdBrowserWidget::ResourceIdBrowserWidget(const std::string& resource_type_filter, QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);

    auto resource_id_label_text = resource_type_filter + " Id:";
    auto resource_id_label = new QLabel(resource_id_label_text.c_str(), this);
    resource_id_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(resource_id_label);

    cbResourceIds = new QComboBox(this);
    cbResourceIds->addItem("");
    cbResourceIds->setEditable(true);
    cbResourceIds->setAutoCompletion(true);
    cbResourceIds->setMinimumWidth(300);
    cbResourceIds->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    reinterpret_cast<QListView*>(cbResourceIds->view())->setUniformItemSizes(true);
    reinterpret_cast<QListView*>(cbResourceIds->view())->setLayoutMode(QListView::Batched);

    auto resource_ids = GlacierFormats::ResourceRepository::instance()->getIdsByType(resource_type_filter);
    std::sort(resource_ids.begin(), resource_ids.end());

    QStringList prim_id_list;
    prim_id_list.reserve(resource_ids.size());
    for (const auto& prim_id : resource_ids)
        prim_id_list.push_back(QString::fromStdString(prim_id));
    cbResourceIds->addItems(prim_id_list);

    layout->addWidget(cbResourceIds);
    connect(cbResourceIds, SIGNAL(editTextChanged(const QString&)), this, SLOT(cbTextChanged(const QString&)));

    setLayout(layout);
}

void ResourceIdBrowserWidget::cbTextChanged(const QString& text_) {
    RuntimeId id = text_.toStdString();
    if (id)
        emit idChanged(id);

}

GlacierFormats::RuntimeId ResourceIdBrowserWidget::id() const {
    const GlacierFormats::RuntimeId id = cbResourceIds->currentText().toStdString();
    if(GlacierFormats::ResourceRepository::instance()->contains(id))
        return id;
    return GlacierFormats::RuntimeId(0);
}
