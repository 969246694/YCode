#include "MainWindow.h"
#include "ChatWidget.h"
#include "CodeEditor.h"
#include "AgentManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QLabel>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCloseEvent>
#include <QSplitter>
#include <QTabBar>
#include <QIcon>
#include <QShortcut>
#include <QInputDialog>
#include <QApplication>
#include <QCoreApplication>
#include <QMenuBar>
#include <QScrollBar>
#include <QTextBlock>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QDebug>
#include <QTimer>
#include <QProcessEnvironment>
#include <QVector>

namespace {

QString panelColorButtonStyle(const QColor &color)
{
    return QString(
        "QPushButton {"
        "    background: %1;"
        "    color: %2;"
        "    border: 1px solid #777777;"
        "    border-radius: 3px;"
        "    padding: 6px 12px;"
        "    min-width: 88px;"
        "}")
        .arg(color.name(), color.lightness() < 128 ? "#FFFFFF" : "#111111");
}

bool pickPanelThemeColor(QWidget *parent, const QString &title, QString &colorValue, QPushButton *button)
{
    QColor current(colorValue);
    QColor chosen = QColorDialog::getColor(current.isValid() ? current : QColor("#007ACC"), parent, title);
    if (!chosen.isValid())
        return false;

    colorValue = chosen.name(QColor::HexRgb).toUpper();
    button->setText(colorValue);
    button->setStyleSheet(panelColorButtonStyle(chosen));
    return true;
}

} // namespace

// ============================================================
// 构造函数 & 析构函数
// ============================================================

void MainWindow::showPanelThemeSettings()
{
    PanelTheme editingTheme = panelTheme;

    QDialog dialog(this);
    dialog.setWindowTitle("面板主题");
    dialog.setModal(true);
    dialog.setMinimumWidth(420);
    dialog.setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; }"
        "QComboBox { background: %3; color: %2; border: 1px solid %4; padding: 4px 8px; }"
        "QPushButton { background: %5; color: #FFFFFF; border: none; border-radius: 3px; padding: 6px 12px; }"
        "QPushButton:hover { background: %6; }")
        .arg(panelTheme.elevatedBackground,
             panelTheme.textColor,
             panelTheme.panelBackground,
             panelTheme.borderColor,
             panelTheme.accentColor,
             panelTheme.borderColor));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    QComboBox *presetCombo = new QComboBox(&dialog);
    presetCombo->addItem("深色工作台", "dark");
    presetCombo->addItem("亮色面板", "light");
    presetCombo->addItem("午夜蓝", "midnight");
    presetCombo->addItem("森林绿", "forest");
    presetCombo->addItem("自定义", "custom");
    int presetIndex = presetCombo->findData(editingTheme.preset);
    if (presetIndex >= 0)
        presetCombo->setCurrentIndex(presetIndex);

    struct ColorButtonBinding {
        QString PanelTheme::*member;
        QPushButton *button;
    };
    QVector<ColorButtonBinding> colorButtons;

    auto addColorRow = [&](const QString &label, QString PanelTheme::*member) {
        QWidget *row = new QWidget(&dialog);
        QHBoxLayout *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *nameLabel = new QLabel(label, row);
        QPushButton *button = new QPushButton(editingTheme.*member, row);
        button->setStyleSheet(panelColorButtonStyle(QColor(editingTheme.*member)));
        rowLayout->addWidget(nameLabel, 1);
        rowLayout->addWidget(button);
        layout->addWidget(row);
        colorButtons.append(ColorButtonBinding{member, button});

        connect(button, &QPushButton::clicked, this, [&dialog, &editingTheme, presetCombo, member, label, button]() {
            if (pickPanelThemeColor(&dialog, label, editingTheme.*member, button))
                presetCombo->setCurrentIndex(presetCombo->findData("custom"));
        });
    };

    layout->addWidget(new QLabel("预设", &dialog));
    layout->addWidget(presetCombo);
    addColorRow("面板背景", &PanelTheme::panelBackground);
    addColorRow("内容背景", &PanelTheme::surfaceBackground);
    addColorRow("抬升背景", &PanelTheme::elevatedBackground);
    addColorRow("正文颜色", &PanelTheme::textColor);
    addColorRow("弱文本颜色", &PanelTheme::mutedTextColor);
    addColorRow("边框颜色", &PanelTheme::borderColor);
    addColorRow("强调色", &PanelTheme::accentColor);
    addColorRow("终端文字", &PanelTheme::terminalTextColor);

    auto refreshColorButtons = [&editingTheme, &colorButtons]() {
        for (const ColorButtonBinding &binding : colorButtons)
        {
            QString value = editingTheme.*(binding.member);
            binding.button->setText(value);
            binding.button->setStyleSheet(panelColorButtonStyle(QColor(value)));
        }
    };

    connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [presetCombo, &editingTheme, refreshColorButtons]() {
        QString preset = presetCombo->currentData().toString();
        if (preset == "custom")
        {
            editingTheme.preset = "custom";
            refreshColorButtons();
            return;
        }

        editingTheme.preset = preset;
        if (preset == "light")
        {
            editingTheme.panelBackground = "#F3F4F6";
            editingTheme.surfaceBackground = "#FFFFFF";
            editingTheme.elevatedBackground = "#E5E7EB";
            editingTheme.textColor = "#111827";
            editingTheme.mutedTextColor = "#6B7280";
            editingTheme.borderColor = "#CBD5E1";
            editingTheme.accentColor = "#2563EB";
            editingTheme.terminalTextColor = "#166534";
        }
        else if (preset == "midnight")
        {
            editingTheme.panelBackground = "#101827";
            editingTheme.surfaceBackground = "#0B1120";
            editingTheme.elevatedBackground = "#172033";
            editingTheme.textColor = "#D7E0EA";
            editingTheme.mutedTextColor = "#91A4B7";
            editingTheme.borderColor = "#27364C";
            editingTheme.accentColor = "#38BDF8";
            editingTheme.terminalTextColor = "#A7F3D0";
        }
        else if (preset == "forest")
        {
            editingTheme.panelBackground = "#16211B";
            editingTheme.surfaceBackground = "#0F1713";
            editingTheme.elevatedBackground = "#243329";
            editingTheme.textColor = "#DDE7DE";
            editingTheme.mutedTextColor = "#96A89B";
            editingTheme.borderColor = "#385044";
            editingTheme.accentColor = "#4ADE80";
            editingTheme.terminalTextColor = "#BBF7D0";
        }
        else
        {
            editingTheme.panelBackground = "#252526";
            editingTheme.surfaceBackground = "#1E1E1E";
            editingTheme.elevatedBackground = "#2D2D30";
            editingTheme.textColor = "#CCCCCC";
            editingTheme.mutedTextColor = "#999999";
            editingTheme.borderColor = "#3C3C3C";
            editingTheme.accentColor = "#007ACC";
            editingTheme.terminalTextColor = "#00FF00";
        }
        refreshColorButtons();
    });

    QHBoxLayout *actions = new QHBoxLayout();
    QPushButton *cancelButton = new QPushButton("取消", &dialog);
    QPushButton *applyButton = new QPushButton("应用", &dialog);
    actions->addStretch();
    actions->addWidget(cancelButton);
    actions->addWidget(applyButton);
    layout->addLayout(actions);

    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted)
        return;

    if (presetCombo->currentData().toString() == "custom")
        editingTheme.preset = "custom";

    panelTheme = editingTheme;
    applyPanelTheme();
    saveSettings();
    statusMessage->setText("面板主题已更新: " + panelThemePresetName(panelTheme.preset));
}

// ============================================================
// 命令面板快捷键
// ============================================================

bool MainWindow::reloadStyleSheet(bool notifyUser)
{
    QString stylePath = QDir(currentProjectPath).filePath("YZCodex/resources/style.qss");
    if (!QFileInfo::exists(stylePath))
    {
        if (notifyUser)
        {
            appendToChat("样式热加载失败：找不到 " + stylePath, false);
            statusMessage->setText("样式热加载失败");
        }
        return false;
    }

    QFile styleFile(stylePath);
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (notifyUser)
        {
            appendToChat("样式热加载失败：无法读取 " + stylePath, false);
            statusMessage->setText("样式热加载失败");
        }
        return false;
    }

    qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    applyPanelTheme();
    if (notifyUser)
    {
        appendToChat("样式已热加载: " + stylePath, false);
        statusMessage->setText("样式已热加载");
    }
    return true;
}

QString MainWindow::panelThemePresetName(const QString &presetKey) const
{
    if (presetKey == "light")
        return "亮色面板";
    if (presetKey == "midnight")
        return "午夜蓝";
    if (presetKey == "forest")
        return "森林绿";
    if (presetKey == "custom")
        return "自定义";
    return "深色工作台";
}

void MainWindow::applyPanelThemeToEditor(CodeEditor *editor)
{
    if (!editor)
        return;

    editor->setStyleSheet(QString(
        "QPlainTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    selection-background-color: %4;"
        "    selection-color: #FFFFFF;"
        "    font-family: 'Consolas', 'Cascadia Code', 'Monaco', 'Courier New', monospace;"
        "}")
        .arg(panelTheme.surfaceBackground,
             panelTheme.textColor,
             panelTheme.borderColor,
             panelTheme.accentColor));
}

void MainWindow::applyPanelTheme()
{
    QString titleStyle = QString(
        "QLabel {"
        "    color: %1;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    padding: 10px 0px;"
        "    letter-spacing: 0.5px;"
        "}")
        .arg(panelTheme.textColor);

    if (sidebarTitle)
        sidebarTitle->setStyleSheet(titleStyle);
    if (chatTitle)
        chatTitle->setStyleSheet(titleStyle);

    if (leftSidebar)
        leftSidebar->setStyleSheet(QString("QWidget { background: %1; color: %2; }").arg(panelTheme.panelBackground, panelTheme.textColor));
    if (chatPanel)
        chatPanel->setStyleSheet(QString("QWidget { background: %1; color: %2; }").arg(panelTheme.panelBackground, panelTheme.textColor));
    if (editorArea)
        editorArea->setStyleSheet(QString("QWidget { background: %1; color: %2; }").arg(panelTheme.surfaceBackground, panelTheme.textColor));
    if (bottomPanel)
        bottomPanel->setStyleSheet(QString("QWidget { background: %1; color: %2; }").arg(panelTheme.surfaceBackground, panelTheme.textColor));

    if (activityBar)
    {
        activityBar->setStyleSheet(QString(
            "QToolBar {"
            "    background: %1;"
            "    border: none;"
            "    spacing: 4px;"
            "    padding: 8px 0px;"
            "}"
            "QToolButton {"
            "    background: transparent;"
            "    border: none;"
            "    border-left: 2px solid transparent;"
            "    padding: 8px;"
            "    margin: 2px 0px;"
            "    color: %2;"
            "    font-size: 20px;"
            "}"
            "QToolButton:hover {"
            "    color: %3;"
            "    background: %4;"
            "}"
            "QToolButton:checked, QToolButton[active=\"true\"] {"
            "    color: %3;"
            "    border-left: 2px solid %5;"
            "    background: %4;"
            "}")
            .arg(panelTheme.elevatedBackground,
                 panelTheme.mutedTextColor,
                 panelTheme.textColor,
                 panelTheme.panelBackground,
                 panelTheme.accentColor));
    }

    QString tabStyle = QString(
        "QTabWidget::pane { border: none; background: %1; }"
        "QTabBar::tab {"
        "    background: transparent;"
        "    color: %2;"
        "    padding: 6px 16px;"
        "    border: none;"
        "    font-size: 12px;"
        "}"
        "QTabBar::tab:selected {"
        "    color: %3;"
        "    border-bottom: 2px solid %4;"
        "}"
        "QTabBar::tab:hover { background: %5; }")
        .arg(panelTheme.surfaceBackground,
             panelTheme.mutedTextColor,
             panelTheme.textColor,
             panelTheme.accentColor,
             panelTheme.elevatedBackground);

    if (leftSidebarTabs)
        leftSidebarTabs->setStyleSheet(tabStyle);
    if (bottomTabs)
        bottomTabs->setStyleSheet(tabStyle);
    if (editorTabs)
    {
        editorTabs->setStyleSheet(QString(
            "QTabWidget::pane { border: none; background: %1; }"
            "QTabBar::tab {"
            "    background: %2;"
            "    color: %3;"
            "    padding: 6px 12px;"
            "    margin-right: 2px;"
            "    border: none;"
            "    font-size: 12px;"
            "}"
            "QTabBar::tab:selected {"
            "    background: %1;"
            "    color: %4;"
            "    border-top: 2px solid %5;"
            "}"
            "QTabBar::tab:hover { background: %6; }")
            .arg(panelTheme.surfaceBackground,
                 panelTheme.elevatedBackground,
                 panelTheme.mutedTextColor,
                 panelTheme.textColor,
                 panelTheme.accentColor,
                 panelTheme.panelBackground));
    }

    if (editorSplitter)
        editorSplitter->setStyleSheet(QString("QSplitter::handle { background: %1; height: 2px; width: 2px; }").arg(panelTheme.borderColor));
    if (mainSplitter)
        mainSplitter->setStyleSheet(QString("QSplitter::handle { background: %1; height: 2px; width: 2px; }").arg(panelTheme.borderColor));

    QString inputStyle = QString(
        "QLineEdit {"
        "    background: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 4px;"
        "    padding: 8px 12px;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid %4; }")
        .arg(panelTheme.elevatedBackground,
             panelTheme.textColor,
             panelTheme.borderColor,
             panelTheme.accentColor);

    QString compactInputStyle = QString(
        "QLineEdit {"
        "    background: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    padding: 4px 8px;"
        "    font-size: 13px;"
        "}"
        "QLineEdit:focus { border: 1px solid %4; }")
        .arg(panelTheme.elevatedBackground,
             panelTheme.textColor,
             panelTheme.borderColor,
             panelTheme.accentColor);

    if (searchBar)
        searchBar->setStyleSheet(QString("QWidget { background-color: %1; border-bottom: 1px solid %2; }").arg(panelTheme.panelBackground, panelTheme.borderColor));
    if (searchInput)
        searchInput->setStyleSheet(compactInputStyle);
    if (replaceInput)
        replaceInput->setStyleSheet(compactInputStyle);
    if (searchGlobalInput)
        searchGlobalInput->setStyleSheet(inputStyle);
    if (inputField)
        inputField->setStyleSheet(inputStyle);
    if (chatInputArea)
        chatInputArea->setStyleSheet(QString("QWidget { background: %1; border-top: 1px solid %2; }").arg(panelTheme.panelBackground, panelTheme.borderColor));

    QString buttonStyle = QString(
        "QPushButton {"
        "    background: %1;"
        "    color: #FFFFFF;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:pressed { background: %3; }")
        .arg(panelTheme.accentColor,
             panelTheme.borderColor,
             panelTheme.panelBackground);

    if (sendButton)
        sendButton->setStyleSheet(buttonStyle);

    QString smallButtonStyle = QString(
        "QPushButton {"
        "    background: %1;"
        "    color: %2;"
        "    border: 1px solid %3;"
        "    border-radius: 3px;"
        "    padding: 4px 10px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover { background: %4; }"
        "QPushButton:pressed { background: %5; color: #FFFFFF; }")
        .arg(panelTheme.elevatedBackground,
             panelTheme.textColor,
             panelTheme.borderColor,
             panelTheme.panelBackground,
             panelTheme.accentColor);

    if (btnSearchNext)
        btnSearchNext->setStyleSheet(smallButtonStyle);
    if (btnSearchPrev)
        btnSearchPrev->setStyleSheet(smallButtonStyle);
    if (btnReplace)
        btnReplace->setStyleSheet(smallButtonStyle);
    if (btnReplaceAll)
        btnReplaceAll->setStyleSheet(smallButtonStyle);
    if (btnCloseSearch)
        btnCloseSearch->setStyleSheet(smallButtonStyle);

    if (fileTreeView)
    {
        fileTreeView->setStyleSheet(QString(
            "QTreeView {"
            "    background: %1;"
            "    color: %2;"
            "    border: none;"
            "    font-size: 13px;"
            "    outline: none;"
            "}"
            "QTreeView::item { padding: 4px 8px; border: none; }"
            "QTreeView::item:hover { background: %3; }"
            "QTreeView::item:selected { background: %4; color: %5; }")
            .arg(panelTheme.panelBackground,
                 panelTheme.textColor,
                 panelTheme.elevatedBackground,
                 panelTheme.accentColor,
                 "#FFFFFF"));
    }

    QString plainTextStyle = QString(
        "QPlainTextEdit {"
        "    background: %1;"
        "    color: %2;"
        "    border: none;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "    font-size: 12px;"
        "    padding: 8px;"
        "    selection-background-color: %3;"
        "}")
        .arg(panelTheme.surfaceBackground,
             panelTheme.textColor,
             panelTheme.accentColor);

    if (terminalOutput)
    {
        terminalOutput->setStyleSheet(QString(
            "QPlainTextEdit {"
            "    background: %1;"
            "    color: %2;"
            "    border: none;"
            "    font-family: 'Consolas', 'Courier New', monospace;"
            "    font-size: 12px;"
            "    padding: 8px;"
            "    selection-background-color: %3;"
            "}")
            .arg(panelTheme.surfaceBackground,
                 panelTheme.terminalTextColor,
                 panelTheme.accentColor));
    }
    if (terminalInput)
    {
        terminalInput->setStyleSheet(QString(
            "QLineEdit {"
            "    background: %1;"
            "    color: %2;"
            "    border: none;"
            "    border-top: 1px solid %3;"
            "    padding: 6px 8px;"
            "    font-family: 'Consolas', 'Courier New', monospace;"
            "    font-size: 12px;"
            "}")
            .arg(panelTheme.panelBackground,
                 panelTheme.terminalTextColor,
                 panelTheme.borderColor));
    }
    if (outputLog)
        outputLog->setStyleSheet(plainTextStyle);
    if (problemsList)
    {
        problemsList->setStyleSheet(QString(
            "QListWidget {"
            "    background: %1;"
            "    color: %2;"
            "    border: none;"
            "    font-size: 12px;"
            "}"
            "QListWidget::item { padding: 4px 8px; }"
            "QListWidget::item:hover { background: %3; }"
            "QListWidget::item:selected { background: %4; color: #FFFFFF; }")
            .arg(panelTheme.surfaceBackground,
                 panelTheme.textColor,
                 panelTheme.elevatedBackground,
                 panelTheme.accentColor));
    }

    if (chatDisplay)
    {
        chatDisplay->applyTheme(panelTheme.surfaceBackground,
                                panelTheme.elevatedBackground,
                                panelTheme.textColor,
                                panelTheme.mutedTextColor,
                                panelTheme.borderColor,
                                panelTheme.accentColor);
    }

    if (statusBar)
    {
        statusBar->setStyleSheet(QString(
            "QStatusBar {"
            "    background: %1;"
            "    color: #FFFFFF;"
            "    border: none;"
            "    padding: 2px 8px;"
            "    font-size: 12px;"
            "}"
            "QStatusBar::item { border: none; }")
            .arg(panelTheme.accentColor));
    }

    const QString statusLabelStyle = "color: #FFFFFF; padding: 0px 12px;";
    if (statusMessage)
        statusMessage->setStyleSheet("color: #FFFFFF; padding: 0px 8px;");
    if (lineColLabel)
        lineColLabel->setStyleSheet(statusLabelStyle);
    if (languageLabel)
        languageLabel->setStyleSheet(statusLabelStyle);
    if (encodingLabel)
        encodingLabel->setStyleSheet(statusLabelStyle);

    if (menuBar())
    {
        menuBar()->setStyleSheet(QString(
            "QMenuBar { background: %1; color: %2; border: none; padding: 2px; }"
            "QMenuBar::item { padding: 4px 12px; }"
            "QMenuBar::item:selected { background: %3; }"
            "QMenu { background: %4; color: %2; border: 1px solid %5; padding: 4px; }"
            "QMenu::item { padding: 6px 30px 6px 20px; }"
            "QMenu::item:selected { background: %6; color: #FFFFFF; }"
            "QMenu::separator { height: 1px; background: %5; margin: 4px 10px; }")
            .arg(panelTheme.elevatedBackground,
                 panelTheme.textColor,
                 panelTheme.panelBackground,
                 panelTheme.surfaceBackground,
                 panelTheme.borderColor,
                 panelTheme.accentColor));
    }

    for (int i = 0; editorTabs && i < editorTabs->count(); ++i)
        applyPanelThemeToEditor(qobject_cast<CodeEditor *>(editorTabs->widget(i)));
}

void MainWindow::loadPanelTheme(QSettings &settings)
{
    panelTheme.preset = settings.value("panelTheme/preset", panelTheme.preset).toString();
    panelTheme.panelBackground = settings.value("panelTheme/panelBackground", panelTheme.panelBackground).toString();
    panelTheme.surfaceBackground = settings.value("panelTheme/surfaceBackground", panelTheme.surfaceBackground).toString();
    panelTheme.elevatedBackground = settings.value("panelTheme/elevatedBackground", panelTheme.elevatedBackground).toString();
    panelTheme.textColor = settings.value("panelTheme/textColor", panelTheme.textColor).toString();
    panelTheme.mutedTextColor = settings.value("panelTheme/mutedTextColor", panelTheme.mutedTextColor).toString();
    panelTheme.borderColor = settings.value("panelTheme/borderColor", panelTheme.borderColor).toString();
    panelTheme.accentColor = settings.value("panelTheme/accentColor", panelTheme.accentColor).toString();
    panelTheme.terminalTextColor = settings.value("panelTheme/terminalTextColor", panelTheme.terminalTextColor).toString();
}

void MainWindow::savePanelTheme(QSettings &settings) const
{
    settings.setValue("panelTheme/preset", panelTheme.preset);
    settings.setValue("panelTheme/panelBackground", panelTheme.panelBackground);
    settings.setValue("panelTheme/surfaceBackground", panelTheme.surfaceBackground);
    settings.setValue("panelTheme/elevatedBackground", panelTheme.elevatedBackground);
    settings.setValue("panelTheme/textColor", panelTheme.textColor);
    settings.setValue("panelTheme/mutedTextColor", panelTheme.mutedTextColor);
    settings.setValue("panelTheme/borderColor", panelTheme.borderColor);
    settings.setValue("panelTheme/accentColor", panelTheme.accentColor);
    settings.setValue("panelTheme/terminalTextColor", panelTheme.terminalTextColor);
}
