#include "mainwindow.h"
#include "GlacierFormats.h"
#include "ui_mainwindow.h"
#include "Console.h"
#include "textureImport.h"
#include "materialEditorWidget.h"

#include <regex>
#include <Windows.h>

#include <QtWidgets>
#include <QtConcurrent/qtconcurrentrun.h>

using namespace GlacierFormats;

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle(tr("Glacier Prim I/O tool"));

    QGridLayout* layout = new QGridLayout(this);

    runtimeDirectory = new PathBrowserWidget(PathBrowserType::OPEN_DIRECTORY, "Runtime directory:", "", this);
    runtimeDirectory->setPath(QString::fromStdString(GlacierFormats::ResourceRepository::runtime_dir.generic_string()));
    runtimeDirectory->setEnabled(false);
    layout->addWidget(runtimeDirectory, 0, 0, 1, 3);

    exportWidget = new PrimExportWidget(this);
    //layout->addWidget(exportWidget, 1, 0, 1, 1);

    importWidget = new GltfImportWidget(this);
    //layout->addWidget(importWidget, 1, 2, 1, 1);

    auto tabs = new QTabWidget(this);
    layout->addWidget(tabs, 1, 0, 1, 3);

    tabs->addTab(exportWidget, "Prim Export" );
    tabs->addTab(importWidget, "Prim Import" );
    tabs->addTab(new materialEditorWidget(this), "Material Editor");
    //tabs->addTab(new TextureToolWidget(this), "Texture Tool" );
    //tabs->addTab(new QWidget(this), "Patch Tool" );

    console = new ConsoleWidget(this);
    console->setMaximumHeight(250);
    Console::instance().setDestinationWidget(console);
    printStatus("Glacier PRIM I/O v1.04 by B3\n");
    layout->addWidget(console, 2, 0, 1, 3);

    auto footer = new FooterWidget(this);
    connect(importWidget, SIGNAL(importStarted()), footer, SLOT(startSpinner()));
    connect(importWidget, SIGNAL(importFinished()), footer, SLOT(stopSpinner()));
    connect(exportWidget, SIGNAL(exportStarted()), footer, SLOT(startSpinner()));
    connect(exportWidget, SIGNAL(exportFinished()), footer, SLOT(stopSpinner()));
    layout->addWidget(footer, 3, 0, 1, 3);
}

QString getLoadingGifPath() {
    TCHAR str[512];
    GetModuleFileNameA(NULL, str, sizeof(str));
    auto path = std::filesystem::path(std::string(str)).parent_path() / "res\\loading.gif";
    return QString::fromStdString(path.lexically_normal().generic_string());
}

FooterWidget::FooterWidget(QWidget* parent) : QWidget(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);

    auto margins = layout->contentsMargins();
    margins.setTop(0);
    margins.setBottom(0);
    layout->setContentsMargins(margins);

    spinnerLabel = new QLabel(this);
    auto gifPath = getLoadingGifPath();
    QMovie* movie = new QMovie(gifPath, QByteArray(), this);
    spinnerLabel->setMovie(movie);
    spinnerLabel->movie()->start();
    spinnerLabel->movie()->stop();
    spinnerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(spinnerLabel);

    auto exitButton = new QPushButton("Exit", this);
    connect(exitButton, SIGNAL(clicked()), SLOT(exitClicked()));
    layout->addWidget(exitButton);

    setLayout(layout);
}

void FooterWidget::exitClicked() {
    exit(0);
}

void FooterWidget::startSpinner() {
    spinnerLabel->movie()->start();
}

void FooterWidget::stopSpinner() {
    spinnerLabel->movie()->stop();
    spinnerLabel->movie()->start();
    spinnerLabel->movie()->stop();
}