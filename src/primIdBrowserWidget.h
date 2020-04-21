#pragma once
#include <QtWidgets>

namespace GlacierFormats {
    class RuntimeId;
}

class ResourceIdBrowserWidget : public QWidget {
    Q_OBJECT

public:
    ResourceIdBrowserWidget(const std::string& resource_type_filter, QWidget* parent = nullptr);

    GlacierFormats::RuntimeId id() const;

private:
    QComboBox* cbResourceIds;

private slots:
    void cbTextChanged(const QString& text);

signals:
    void idChanged(const std::string& id);
};