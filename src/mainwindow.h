#pragma once

#include "Console.h"
#include "primImport.h"
#include "primExport.h"

#include <filesystem>

#include <QWidget>
#include <QtWidgets>
#include <QDir>
#include <QMainWindow>

class MainWindow : public QWidget {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

private:
    PathBrowserWidget* runtimeDirectory;
    PrimExportWidget* exportWidget;
    GltfImportWidget* importWidget;
    ConsoleWidget* console;
};

class FooterWidget : public QWidget {
    Q_OBJECT

private:
    QLabel* spinnerLabel;

public:
    FooterWidget(QWidget* parent);

private slots:
    void exitClicked();

public slots:
    void startSpinner();
    void stopSpinner();
};
