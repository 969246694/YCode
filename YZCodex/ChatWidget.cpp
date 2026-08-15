#include "ChatWidget.h"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QScrollBar>
#include <QTimer>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QToolButton>
#include <QApplication>
#include <QClipboard>

namespace {

// 新消息淡入（200ms 透明度 0->1），完成后移除特效避免影响渲染
void fadeInWidget(QWidget *widget)
{
    if (!widget)
        return;
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", widget);
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    QObject::connect(anim, &QPropertyAnimation::finished, effect, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// 平滑滚动到底部（250ms OutCubic），避免生硬跳变
void scrollToBottomSmooth(QScrollArea *scrollArea)
{
    if (!scrollArea)
        return;
    QScrollBar *bar = scrollArea->verticalScrollBar();
    const int target = bar->maximum();
    const int current = bar->value();
    if (target <= current)
        return;
    QPropertyAnimation *anim = new QPropertyAnimation(bar, "value", scrollArea);
    anim->setDuration(250);
    anim->setStartValue(current);
    anim->setEndValue(target);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

} // namespace

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
    fadeInWidget(messageWidget);

    // 平滑滚动到底部
    scrollToBottomSmooth(scrollArea);
}

void ChatWidget::appendAssistantMessage(const QString &message)
{
    QWidget *messageWidget = createMessageWidget(message, false);
    scrollLayout->addWidget(messageWidget);
    fadeInWidget(messageWidget);

    // 平滑滚动到底部
    scrollToBottomSmooth(scrollArea);
}

void ChatWidget::beginAssistantMessage()
{
    // 新一轮助手回复开始：后续 appendToLastAssistant 会创建全新气泡
    lastAssistantLabel = nullptr;
}

void ChatWidget::startTypingIndicator()
{
    stopTypingIndicator(); // 幂等：先清理旧的指示器
    typingDotCount = 0;

    typingWidget = new QWidget();
    typingWidget->setStyleSheet("QWidget { background: transparent; }");
    QHBoxLayout *layout = new QHBoxLayout(typingWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    QLabel *avatar = new QLabel("🤖");
    avatar->setProperty("messageRole", "assistantAvatar");
    avatar->setFixedSize(28, 28);
    avatar->setAlignment(Qt::AlignCenter);
    styleAvatarLabel(avatar, false);

    typingLabel = new QLabel("Agent 正在思考");
    typingLabel->setProperty("messageRole", "typingIndicator");
    typingLabel->setMaximumWidth(480);
    typingLabel->setStyleSheet(QString(
        "QLabel {"
        "    background-color: %1;"
        "    color: %2;"
        "    padding: 10px 14px;"
        "    border-radius: 8px;"
        "    font-size: 13px;"
        "    border: 1px solid %3;"
        "}")
        .arg(surfaceBackground, mutedTextColor, borderColor));

    layout->addWidget(avatar);
    layout->addWidget(typingLabel);
    layout->addStretch();

    scrollLayout->addWidget(typingWidget);
    scrollToBottomSmooth(scrollArea);

    typingTimer = new QTimer(this);
    typingTimer->setInterval(300);
    auto renderDots = [this]() {
        // 三圆点弹跳动画：●○○ → ○●○ → ○○●
        QString dots;
        for (int i = 0; i < 3; ++i)
            dots += (i == typingDotCount) ? QStringLiteral("●") : QStringLiteral("○");
        typingLabel->setText(QStringLiteral("Agent 正在思考  ") + dots);
    };
    QObject::connect(typingTimer, &QTimer::timeout, this, [this, renderDots]() {
        typingDotCount = (typingDotCount + 1) % 3;
        renderDots();
    });
    renderDots(); // 立即显示第一帧，避免等待首个 tick
    typingTimer->start();
}

void ChatWidget::stopTypingIndicator()
{
    if (typingTimer)
    {
        typingTimer->stop();
        typingTimer->deleteLater();
        typingTimer = nullptr;
    }
    if (typingWidget)
    {
        scrollLayout->removeWidget(typingWidget);
        typingWidget->deleteLater();
        typingWidget = nullptr;
    }
    typingLabel = nullptr;
}

void ChatWidget::appendToLastAssistant(const QString &delta)
{
    if (!lastAssistantLabel)
    {
        // 首段内容到达：移除“正在思考…”指示器，再创建正式气泡
        stopTypingIndicator();
        appendAssistantMessage(delta);
        return;
    }

    lastAssistantLabel->setText(lastAssistantLabel->text() + delta);
    QScrollBar *scrollBar = scrollArea->verticalScrollBar();
    QTimer::singleShot(50, [scrollBar]()
                       { scrollBar->setValue(scrollBar->maximum()); });
}

void ChatWidget::clear()
{
    stopTypingIndicator();
    QLayoutItem *item;
    while ((item = scrollLayout->takeAt(0)) != nullptr)
    {
        if (item->widget())
        {
            delete item->widget();
        }
        delete item;
    }
    lastAssistantLabel = nullptr;
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
    label->setAttribute(Qt::WA_Hover, true); // 启用 QSS :hover 伪状态
    label->setStyleSheet(QString(
        "QLabel {"
        "    background-color: %1;"
        "    color: %2;"
        "    padding: 10px 14px;"
        "    border-radius: 8px;"
        "    font-size: 13px;"
        "    line-height: 1.5;"
        "    border: 1px solid %3;"
        "}"
        "QLabel:hover {"
        "    border: 1px solid %4;"
        "    background-color: %5;"
        "}")
        .arg(isUser ? accentColor : surfaceBackground,
             isUser ? "#FFFFFF" : textColor,
             isUser ? accentColor : borderColor,
             isUser ? "#3399FF" : accentColor,
             isUser ? accentColor : QString("#323238")));
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
    messageLabel->setMaximumWidth(480);
    styleMessageLabel(messageLabel, isUser);

    // 记录最近一个助手消息的文本标签，供流式追加使用
    lastAssistantLabel = isUser ? nullptr : messageLabel;

    // 消息时间标签
    QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm"));
    timeLabel->setProperty("messageRole", "time");
    styleTimeLabel(timeLabel);
    timeLabel->setAlignment(Qt::AlignBottom | (isUser ? Qt::AlignRight : Qt::AlignLeft));

    // 复制按钮：一键复制消息内容（对代码片段尤其有用）
    QToolButton *copyButton = new QToolButton();
    copyButton->setText("复制");
    copyButton->setToolTip("复制消息内容");
    copyButton->setCursor(Qt::PointingHandCursor);
    copyButton->setStyleSheet(
        "QToolButton {"
        "    background: transparent;"
        "    color: #777777;"
        "    border: none;"
        "    font-size: 10px;"
        "    padding: 2px 6px;"
        "    border-radius: 3px;"
        "}"
        "QToolButton:hover { color: #007ACC; background: #2A2D2E; }");
    QObject::connect(copyButton, &QToolButton::clicked, this, [messageLabel, copyButton]() {
        QApplication::clipboard()->setText(messageLabel->text());
        copyButton->setText("已复制");
        QTimer::singleShot(1200, copyButton, [copyButton]() { copyButton->setText("复制"); });
    });

    // 组装布局
    QVBoxLayout *bubbleLayout = new QVBoxLayout();
    bubbleLayout->setContentsMargins(0, 0, 0, 0);
    bubbleLayout->setSpacing(4);
    bubbleLayout->addWidget(messageLabel);
    QHBoxLayout *metaRow = new QHBoxLayout();
    metaRow->setContentsMargins(0, 0, 0, 0);
    metaRow->setSpacing(6);
    if (isUser)
    {
        // 用户消息：时间靠右，复制按钮紧随其后
        metaRow->addStretch();
        metaRow->addWidget(timeLabel);
        metaRow->addWidget(copyButton);
    }
    else
    {
        // 助手消息：时间靠左，复制按钮靠右
        metaRow->addWidget(timeLabel);
        metaRow->addStretch();
        metaRow->addWidget(copyButton);
    }
    bubbleLayout->addLayout(metaRow);

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
