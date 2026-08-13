#include "ChatWidget.h"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollBar>
#include <QTimer>
#include <QDateTime>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      panelBackground("#1E1E1E"),
      surfaceBackground("#2D2D30"),
      textColor("#CCCCCC"),
      mutedTextColor("#666666"),
      borderColor("#3C3C3C"),
      accentColor("#007ACC"),
      assistantAccentColor("#10B981")
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

    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollWidget = new QWidget();

    scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(10, 10, 10, 10);
    scrollLayout->setSpacing(12);
    scrollLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);
    applyTheme(panelBackground, surfaceBackground, textColor, mutedTextColor, borderColor, accentColor);
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

void ChatWidget::applyTheme(const QString &newPanelBackground,
                            const QString &newSurfaceBackground,
                            const QString &newTextColor,
                            const QString &newMutedTextColor,
                            const QString &newBorderColor,
                            const QString &newAccentColor)
{
    panelBackground = newPanelBackground;
    surfaceBackground = newSurfaceBackground;
    textColor = newTextColor;
    mutedTextColor = newMutedTextColor;
    borderColor = newBorderColor;
    accentColor = newAccentColor;

    setStyleSheet(QString("ChatWidget { background: %1; }").arg(panelBackground));
    scrollArea->setStyleSheet(QString(
        "QScrollArea { border: none; background: %1; }"
        "QScrollBar:vertical {"
        "    background: %1;"
        "    width: 8px;"
        "    margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: %2;"
        "    min-height: 30px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: %3;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}")
        .arg(panelBackground, borderColor, accentColor));
    scrollWidget->setStyleSheet(QString("QWidget { background: %1; }").arg(panelBackground));

    const QList<QLabel *> labels = scrollWidget->findChildren<QLabel *>();
    for (QLabel *label : labels)
    {
        QString role = label->property("messageRole").toString();
        if (role == "userMessage")
            styleMessageLabel(label, true);
        else if (role == "assistantMessage")
            styleMessageLabel(label, false);
        else if (role == "userAvatar")
            styleAvatarLabel(label, true);
        else if (role == "assistantAvatar")
            styleAvatarLabel(label, false);
        else if (role == "time")
            styleTimeLabel(label);
    }
}

void ChatWidget::styleAvatarLabel(QLabel *label, bool isUser) const
{
    label->setStyleSheet(QString(
        "QLabel {"
        "    border-radius: 14px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    background-color: %1;"
        "}")
        .arg(isUser ? accentColor : assistantAccentColor));
}

void ChatWidget::styleMessageLabel(QLabel *label, bool isUser) const
{
    label->setStyleSheet(QString(
        "QLabel {"
        "    background-color: %1;"
        "    color: %2;"
        "    padding: 10px 14px;"
        "    border-radius: 8px;"
        "    font-size: 13px;"
        "    line-height: 1.5;"
        "    border: 1px solid %3;"
        "}")
        .arg(isUser ? accentColor : surfaceBackground,
             isUser ? "#FFFFFF" : textColor,
             isUser ? accentColor : borderColor));
}

void ChatWidget::styleTimeLabel(QLabel *label) const
{
    label->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent;").arg(mutedTextColor));
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
    avatarLabel->setProperty("messageRole", isUser ? "userAvatar" : "assistantAvatar");
    avatarLabel->setFixedSize(28, 28);
    avatarLabel->setAlignment(Qt::AlignCenter);
    styleAvatarLabel(avatarLabel, isUser);
    avatarLabel->setText(isUser ? "👤" : "🤖");

    // 消息气泡
    QLabel *messageLabel = new QLabel(message);
    messageLabel->setProperty("messageRole", isUser ? "userMessage" : "assistantMessage");
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    messageLabel->setMaximumWidth(400);
    styleMessageLabel(messageLabel, isUser);

    // 消息时间标签
    QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm"));
    timeLabel->setProperty("messageRole", "time");
    styleTimeLabel(timeLabel);
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
