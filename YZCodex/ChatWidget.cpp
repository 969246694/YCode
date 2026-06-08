#include "ChatWidget.h"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollBar>
#include <QTimer>
#include <QDateTime>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ChatWidget::~ChatWidget()
{
}

void ChatWidget::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 聊天背景
    setStyleSheet("ChatWidget { background: #1E1E1E; }");

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: #1E1E1E; }"
        "QScrollBar:vertical {"
        "    background: #1E1E1E;"
        "    width: 8px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #424242;"
        "    min-height: 30px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #555555;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
    );

    scrollWidget = new QWidget();
    scrollWidget->setStyleSheet("QWidget { background: #1E1E1E; }");

    scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(10, 10, 10, 10);
    scrollLayout->setSpacing(12);
    scrollLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
}

void ChatWidget::appendUserMessage(const QString &message)
{
    QWidget *messageWidget = createMessageWidget(message, true);
    scrollLayout->addWidget(messageWidget);

    // 自动滚动到底部
    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    QTimer::singleShot(50, [scrollBar]()
                       { scrollBar->setValue(scrollBar->maximum()); });
}

void ChatWidget::appendAssistantMessage(const QString &message)
{
    QWidget *messageWidget = createMessageWidget(message, false);
    scrollLayout->addWidget(messageWidget);

    // 自动滚动到底部
    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    QTimer::singleShot(50, [scrollBar]()
                       { scrollBar->setValue(scrollBar->maximum()); });
}

void ChatWidget::clear()
{
    QLayoutItem *item;
    while ((item = scrollLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            delete item->widget();
        }
        delete item;
    }
}

QWidget *ChatWidget::createMessageWidget(const QString &message, bool isUser)
{
    QWidget *messageWidget = new QWidget();
    messageWidget->setStyleSheet("QWidget { background: transparent; }");

    QHBoxLayout *layout = new QHBoxLayout(messageWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // 头像
    QLabel *avatarLabel = new QLabel();
    avatarLabel->setFixedSize(28, 28);
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(QString(
        "QLabel {"
        "    border-radius: 14px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    background-color: %1;"
        "}")
        .arg(isUser ? "#007ACC" : "#10B981"));
    avatarLabel->setText(isUser ? "👤" : "🤖");

    // 消息气泡
    QLabel *messageLabel = new QLabel(message);
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageLabel->setMaximumWidth(400);
    messageLabel->setStyleSheet(
        "QLabel {"
        "    background-color: " + QString(isUser ? "#094771" : "#2D2D30") + ";"
        "    color: #CCCCCC;"
        "    padding: 10px 14px;"
        "    border-radius: 10px;"
        "    font-size: 13px;"
        "    line-height: 1.5;"
        "    border: 1px solid " + QString(isUser ? "#007ACC" : "#3C3C3C") + ";"
        "}");

    // 消息时间标签
    QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm"));
    timeLabel->setStyleSheet("color: #666666; font-size: 10px; background: transparent;");
    timeLabel->setAlignment(Qt::AlignBottom | (isUser ? Qt::AlignRight : Qt::AlignLeft));

    // 组装布局
    QVBoxLayout *bubbleLayout = new QVBoxLayout();
    bubbleLayout->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->setSpacing(4);
    bubbleLayout->addWidget(messageLabel);
    bubbleLayout->addWidget(timeLabel);

    if (isUser)
    {
        layout->addStretch();
        layout->addLayout(bubbleLayout);
        layout->addWidget(avatarLabel);
    }
    else
    {
        layout->addWidget(avatarLabel);
        layout->addLayout(bubbleLayout);
        layout->addStretch();
    }

    return messageWidget;
}
