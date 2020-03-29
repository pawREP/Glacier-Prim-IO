#pragma once
#include <QtWidgets>

#include <filesystem>

enum class PathBrowserType {
    SAVE_FILE,
    OPEN_FILE,
    OPEN_DIRECTORY
};

class PathBrowserWidget : public QWidget {
    Q_OBJECT

public:
    PathBrowserWidget(PathBrowserType type, const QString& label, const QString& filter = "", QWidget* parent = nullptr);

    QString path() const;
    void setPath(const QString& path);

private:
    PathBrowserType type;
    QString filter;

    QLineEdit* pathLine;
    QPushButton* browseButton;

private slots:
    void browse();

signals:
    void pathChanged();
};

bool isValidOpenFilePath(const std::filesystem::path& path);
bool isValidSaveFilePath(const std::filesystem::path& path);