#pragma once

#include <QWidget>
#include <QtWidgets>

class ConsoleWidget : public QWidget {
    Q_OBJECT

private:
    QTextEdit* text;

public:
    ConsoleWidget(QWidget* parent = nullptr);
    ~ConsoleWidget();

    void printLine(const char* message, const QColor& color);
    void printLine(const std::string& message, const QColor& color);
    void printLine(const QString& message, const QColor& color);

    void print(const char* message, const QColor& color);
    void print(const std::string& message, const QColor& color);
    void print(const QString& message, const QColor& color);
};

class Console {
private:
    ConsoleWidget* consoleWidget;

    Console();

public:
    static Console& instance();

    void setDestinationWidget(ConsoleWidget* widget);

    template<typename T>
    void printLine(T msg, const QColor& color = Qt::white) {
        if (consoleWidget)
            consoleWidget->printLine(msg, color);
    }

    template<typename T>
    void print(T msg, const QColor& color = Qt::white) {
        if (consoleWidget)
            consoleWidget->print(msg, color);
    }

};

void printError(const std::string& msg);
void printStatus(const std::string& msg);