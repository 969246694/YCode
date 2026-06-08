#include "CodeEditor.h"
#include "SyntaxHighlighter.h"
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>
#include <QFontDatabase>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), modified_(false), highlighter(nullptr)
{
    setupUI();
    setupHighlighter();
}

CodeEditor::~CodeEditor()
{
}

void CodeEditor::setupUI()
{
    // 设置等宽字体
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    setFont(font);

    // 设置行号区域
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    // VSCode Dark+ 编辑器样式
    setStyleSheet(
        "QPlainTextEdit {"
        "    background-color: #1E1E1E;"
        "    color: #D4D4D4;"
        "    border: 1px solid #3C3C3C;"
        "    selection-background-color: #264F78;"
        "    selection-color: #FFFFFF;"
        "    font-family: 'Consolas', 'Cascadia Code', 'Monaco', 'Courier New', monospace;"
        "}");

    // 设置制表符宽度 (4 个空格)
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));

    // 启用代码折叠提示（通过缩进）
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // 括号匹配
    connect(this, &CodeEditor::cursorPositionChanged, this, [this]()
            {
        // 简单的括号匹配高亮（在 highlightCurrentLine 中处理）
        highlightCurrentLine(); });
}

void CodeEditor::setupHighlighter()
{
    // 创建 C++ 语法高亮器
    highlighter = new SyntaxHighlighter(document());
}

void CodeEditor::setFilePath(const QString &path)
{
    filePath_ = path;
}

QString CodeEditor::filePath() const
{
    return filePath_;
}

bool CodeEditor::isModified() const
{
    return modified_;
}

void CodeEditor::setModified(bool modified)
{
    modified_ = modified;
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    QPlainTextEdit::keyPressEvent(event);

    // 标记为已修改
    if (!event->text().isEmpty() || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
    {
        setModified(true);
    }
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        // 当前行高亮
        QTextEdit::ExtraSelection lineSelection;
        QColor lineColor = QColor("#2D2D30");
        lineSelection.format.setBackground(lineColor);
        lineSelection.format.setProperty(QTextFormat::FullWidthSelection, true);
        lineSelection.cursor = textCursor();
        lineSelection.cursor.clearSelection();
        extraSelections.append(lineSelection);

        // 括号匹配高亮
        QTextCursor cursor = textCursor();
        QTextBlock block = cursor.block();
        int cursorPos = cursor.positionInBlock();

        // 检查光标位置的字符和它前面的字符
        QString blockText = block.text();

        // 匹配括号对
        struct BracketPair { QChar open; QChar close; };
        QVector<BracketPair> pairs = {
            {'(', ')'}, {'{', '}'}, {'[', ']'}
        };

        for (const BracketPair &pair : pairs)
        {
            QChar charAtCursor = (cursorPos < blockText.length()) ? blockText.at(cursorPos) : QChar();
            QChar charBeforeCursor = (cursorPos > 0) ? blockText.at(cursorPos - 1) : QChar();

            QChar bracket;
            int pos = cursorPos;
            bool isOpen = false;

            if (charAtCursor == pair.open || charAtCursor == pair.close)
            {
                bracket = charAtCursor;
                isOpen = (bracket == pair.open);
                pos = cursorPos;
            }
            else if (charBeforeCursor == pair.open || charBeforeCursor == pair.close)
            {
                bracket = charBeforeCursor;
                isOpen = (bracket == pair.open);
                pos = cursorPos - 1;
            }
            else
            {
                continue;
            }

            // 查找匹配的括号
            int matchPos = -1;
            int depth = 0;

            if (isOpen)
            {
                // 向前搜索匹配的关闭括号
                for (int i = pos; i < blockText.length(); ++i)
                {
                    if (blockText.at(i) == pair.open) depth++;
                    else if (blockText.at(i) == pair.close)
                    {
                        depth--;
                        if (depth == 0) { matchPos = i; break; }
                    }
                }
            }
            else
            {
                // 向后搜索匹配的打开括号
                for (int i = pos; i >= 0; --i)
                {
                    if (blockText.at(i) == pair.close) depth++;
                    else if (blockText.at(i) == pair.open)
                    {
                        depth--;
                        if (depth == 0) { matchPos = i; break; }
                    }
                }
            }

            // 高亮匹配的括号
            if (matchPos >= 0)
            {
                // 高亮当前括号
                QTextEdit::ExtraSelection bracketSel;
                bracketSel.format.setBackground(QColor("#4D4D4D"));
                bracketSel.format.setForeground(QColor("#FFFFFF"));
                QTextCursor bracketCursor(block);
                bracketCursor.setPosition(block.position() + pos);
                bracketCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                bracketSel.cursor = bracketCursor;
                extraSelections.append(bracketSel);

                // 高亮匹配括号
                QTextEdit::ExtraSelection matchSel;
                matchSel.format.setBackground(QColor("#4D4D4D"));
                matchSel.format.setForeground(QColor("#FFFFFF"));
                QTextCursor matchCursor(block);
                matchCursor.setPosition(block.position() + matchPos);
                matchCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                matchSel.cursor = matchCursor;
                extraSelections.append(matchSel);
            }
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);

    // 行号区域背景
    painter.fillRect(event->rect(), QColor("#1E1E1E"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    // 当前行的行号高亮
    int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);

            // 当前行行号使用更亮的颜色
            if (blockNumber == currentLine)
            {
                painter.setPen(QColor("#CCCCCC"));
                painter.setFont(QFont(font().family(), font().pointSize(), QFont::Bold));
            }
            else
            {
                painter.setPen(QColor("#858585"));
                painter.setFont(font());
            }

            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // 绘制缩进参考线（每 4 个空格）
            QString text = block.text();
            int indentLevel = 0;
            for (int i = 0; i < text.length(); ++i)
            {
                if (text.at(i) == ' ')
                    indentLevel++;
                else
                    break;
            }
            indentLevel /= 4;

            if (indentLevel > 0)
            {
                painter.setPen(QColor(0x40, 0x40, 0x40, 60));
                int lineX = lineNumberArea->width() - 2;
                painter.drawLine(lineX, top, lineX, bottom);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }

    // 行号区域右侧分割线
    painter.setPen(QColor("#3C3C3C"));
    int x = lineNumberArea->width() - 1;
    painter.drawLine(x, event->rect().top(), x, event->rect().bottom());
}
