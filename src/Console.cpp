#include "Console.h"

ConsoleWidget::ConsoleWidget(QWidget* parent) : QWidget(parent) {
    text = new QTextEdit(this);

    auto boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(text);
    this->setLayout(boxLayout);
}

ConsoleWidget::~ConsoleWidget() {

}

void ConsoleWidget::printLine(const char* message, const QColor& color) {
    printLine(QString(message), color);
}

void ConsoleWidget::printLine(const std::string& message, const QColor& color) {
    printLine(QString::fromStdString(message), color);
}

void ConsoleWidget::printLine(const QString& message, const QColor& color) {
    text->moveCursor(QTextCursor::End);
    text->setTextColor(color);
    text->append(message);
    text->moveCursor(QTextCursor::End);
}

void ConsoleWidget::print(const char* message, const QColor& color) {
    print(QString(message), color);
}

void ConsoleWidget::print(const std::string& message, const QColor& color) {
    print(QString::fromStdString(message), color);
}

void ConsoleWidget::print(const QString& message, const QColor& color) {
    text->moveCursor(QTextCursor::End);
    text->setTextColor(color);
    text->insertPlainText(message);
    text->moveCursor(QTextCursor::End);
}

Console::Console() {

}

Console& Console::instance() {
    static Console inst;
    return inst;
}

void Console::setDestinationWidget(ConsoleWidget* widget) {
    consoleWidget = widget;
}

void printError(const std::string& msg) {
    Console::instance().printLine("Error: " + msg, Qt::red);
}

void printStatus(const std::string& msg) {
    Console::instance().printLine(msg, Qt::white);
}