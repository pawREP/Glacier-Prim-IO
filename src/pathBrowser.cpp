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
    QString oldPath = path();
    oldPath = oldPath.isEmpty() ? QDir::currentPath() : oldPath;

    QString newPath;
    switch (type) {
    case PathBrowserType::SAVE_FILE:
        newPath = QFileDialog::getSaveFileName(this, "Browse Save File", oldPath, filter);
        break;
    case PathBrowserType::OPEN_FILE:
        newPath = QFileDialog::getOpenFileName(this, "Browse File", oldPath, filter);
        break;
    case PathBrowserType::OPEN_DIRECTORY:
        newPath = QFileDialog::getExistingDirectory(this, "Browse Directory", oldPath);
        break;
    default:
        throw;
    }

    if(!newPath.isEmpty())
        pathLine->setText(newPath);
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