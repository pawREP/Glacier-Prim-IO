#include "mainwindow.h"
#include "GlacierFormats.h"

#include <iostream>
#include <QApplication>
#include <QtConcurrent/qtconcurrentrun.h>

void initAppStyle() {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, Qt::gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipBase, Qt::gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::ToolTipText, Qt::gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::gray);
    darkPalette.setColor(QPalette::Disabled, QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Disabled, QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, Qt::black);

    qApp->setPalette(darkPalette);
    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

void initGlacierFormats() {
    QProgressDialog progressDialog;
    progressDialog.setWindowTitle("Loading...");
    progressDialog.setRange(0, 0);
    progressDialog.setLabel(new QLabel("Initilizing GlacierFormats, this might take a few seconds...", &progressDialog));
    progressDialog.show();

    auto future = QtConcurrent::run(&GlacierFormats::GlacierInit);
    while (!future.isFinished()) {
        qApp->processEvents();
        Sleep(33);
    }
    if (progressDialog.wasCanceled())
        exit(0);

    progressDialog.close();
}

int main(int argc, char *argv[]) {

#ifndef _DEBUG
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    QApplication app(argc, argv);

    initAppStyle();

    initGlacierFormats();

    MainWindow window;
    QSize windowSize(1200, 800);
    window.resize(windowSize);
    window.show();
    return app.exec();
}
