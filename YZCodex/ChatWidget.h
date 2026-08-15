#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QString>
#include <QTimer>

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    void appendUserMessage(const QString &message);
    void appendAssistantMessage(const QString &message);
    void beginAssistantMessage();          // 标记新一轮助手回复开始（后续内容追加到新气泡）
    void appendToLastAssistant(const QString &delta); // 流式追加到当前助手气泡
    void startTypingIndicator();           // 流式回复期间显示“正在思考…”动画
    void stopTypingIndicator();            // 结束/移除正在思考指示器
    void clear();
    void applyTheme(const QString &panelBackground,
                    const QString &surfaceBackground,
                    const QString &textColor,
                    const QString &mutedTextColor,
                    const QString &borderColor,
                    const QString &accentColor);

private:
    void setupUI();
    QWidget *createMessageWidget(const QString &message, bool isUser);
    void styleMessageLabel(QLabel *label, bool isUser) const;
    void styleAvatarLabel(QLabel *label, bool isUser) const;
    void styleTimeLabel(QLabel *label) const;

    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *scrollLayout;
    QString panelBackground;
    QString surfaceBackground;
    QString textColor;
    QString mutedTextColor;
    QString borderColor;
    QString accentColor;
    QString assistantAccentColor;
    QLabel *lastAssistantLabel = nullptr; // 最近一个助手消息的文本标签
    QWidget *typingWidget = nullptr;      // “正在思考…”指示器容器
    QLabel *typingLabel = nullptr;        // “正在思考…”文字
    QTimer *typingTimer = nullptr;        // 圆点动画定时器
    int typingDotCount = 0;               // 当前圆点数量（0..3）
};

#endif
