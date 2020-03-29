#include "pathBrowser.h"

PathBrowserWidget::PathBrowserWidget(PathBrowserType type, const QString& label, const QString& filter, QWidget* parent) : QWidget(parent), filter(filter), type(type) {
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->addWidget(new QLabel(label, this));

    pathLine = new QLineEdit(this);
    pathLine->setText("");
    pathLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(pathLine);

    browseButton = new QPushButton("Browse...", this);
    connect(browseButton, SIGNAL(clicked()), SLOT(browse()));
    layout->addWidget(browseButton);

    setLayout(layout);
}

QString PathBrowserWidget::path() const {
    return pathLine->text();
}

void PathBrowserWidget::setPath(const QString& path) {
    pathLine->setText(path);
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
    pathLine->setText(path);
    emit pathChanged();
}

bool isValidOpenFilePath(const std::filesystem::path& path) {
    if (path.empty())
        return false;
    if (!std::filesystem::exists(path))
        return false;
    if (!std::filesystem::is_regular_file(path))
        return false;
    return true;
}

bool isValidSaveFilePath(const std::filesystem::path& path) {
    if (path.empty())
        return false;
    if (!path.has_parent_path())
        return false;
    if (!std::filesystem::is_directory(path.parent_path()))
        return false;
    if (!path.has_filename())
        return false;
    if (!path.has_extension())
        return false;
    return true;
}