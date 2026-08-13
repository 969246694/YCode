#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QString>

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    void appendUserMessage(const QString &message);
    void appendAssistantMessage(const QString &message);
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
};

#endif
