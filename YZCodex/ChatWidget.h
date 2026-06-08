#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    void appendUserMessage(const QString &message);
    void appendAssistantMessage(const QString &message);
    void clear();

private:
    void setupUI();
    QWidget *createMessageWidget(const QString &message, bool isUser);

    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *scrollLayout;
};

#endif