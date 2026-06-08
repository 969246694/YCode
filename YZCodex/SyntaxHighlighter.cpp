#include "SyntaxHighlighter.h"

// ============================================================
// VSCode Dark+ 配色方案
// ============================================================
// 关键字:        #569CD6 (蓝色)
// 类型/类名:     #4EC9B0 (青绿色)
// 函数名:        #DCDCAA (黄色)
// 字符串:        #CE9178 (橙色)
// 数字:          #B5CEA8 (浅绿色)
// 单行注释:      #6A9955 (绿色)
// 预处理器:      #C586C0 (紫色)
// 操作符/标点:   #D4D4D4 (白色)
// 转义字符:      #D7BA7D (金色)
// 背景:          #1E1E1E (深色)
// 当前行:        #2D2D30
// ============================================================

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // ============ 1. C++ 关键字 (蓝色) ============
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#569CD6"));
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywordPatterns = {
        // 控制流
        "\\bif\\b", "\\belse\\b", "\\bswitch\\b", "\\bcase\\b",
        "\\bdefault\\b", "\\bbreak\\b", "\\bcontinue\\b",
        "\\bfor\\b", "\\bwhile\\b", "\\bdo\\b", "\\bgoto\\b",
        "\\breturn\\b", "\\btry\\b", "\\bcatch\\b", "\\bthrow\\b",

        // 类型修饰符
        "\\bconst\\b", "\\bstatic\\b", "\\bvolatile\\b", "\\bmutable\\b",
        "\\bextern\\b", "\\bregister\\b", "\\bexplicit\\b",
        "\\binline\\b", "\\bvirtual\\b", "\\boverride\\b", "\\bfinal\\b",
        "\\bconstexpr\\b", "\\bconsteval\\b", "\\bnoexcept\\b",

        // 类型关键字
        "\\bvoid\\b", "\\bbool\\b", "\\bchar\\b", "\\bint\\b",
        "\\bfloat\\b", "\\bdouble\\b", "\\blong\\b", "\\bshort\\b",
        "\\bsigned\\b", "\\bunsigned\\b", "\\bwchar_t\\b",
        "\\bauto\\b", "\\bdecltype\\b", "\\btypename\\b",
        "\\btemplate\\b", "\\bclass\\b", "\\bstruct\\b", "\\bunion\\b",
        "\\benum\\b", "\\bnamespace\\b", "\\btypedef\\b", "\\busing\\b",
        "\\boperator\\b", "\\bsizeof\\b", "\\btypeid\\b",
        "\\bnew\\b", "\\bdelete\\b", "\\bthis\\b",
        "\\bfriend\\b", "\\bpublic\\b", "\\bprivate\\b", "\\bprotected\\b",

        // 其他
        "\\btrue\\b", "\\bfalse\\b", "\\bnullptr\\b", "\\bNULL\\b",
        "\\band\\b", "\\bor\\b", "\\bnot\\b", "\\bxor\\b",
        "\\bstatic_cast\\b", "\\bdynamic_cast\\b", "\\bconst_cast\\b",
        "\\breinterpret_cast\\b", "\\balignas\\b", "\\balignof\\b",
    };

    for (const QString &pattern : keywordPatterns)
    {
        highlightingRules.append({QRegularExpression(pattern), keywordFormat});
    }

    // ============ 2. Qt 关键字/宏 (蓝色) ============
    QTextCharFormat qtKeywordFormat;
    qtKeywordFormat.setForeground(QColor("#569CD6"));
    qtKeywordFormat.setFontWeight(QFont::Bold);

    QStringList qtPatterns = {
        "\\bQ_OBJECT\\b", "\\bsignals\\b", "\\bslots\\b",
        "\\bemit\\b", "\\bSIGNAL\\b", "\\bSLOT\\b",
        "\\bQ_PROPERTY\\b", "\\bQ_DISABLE_COPY\\b",
        "\\bQ_DECLARE_METATYPE\\b",
    };

    for (const QString &pattern : qtPatterns)
    {
        highlightingRules.append({QRegularExpression(pattern), qtKeywordFormat});
    }

    // ============ 3. 数字 (浅绿色) ============
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#B5CEA8"));

    highlightingRules.append({
        QRegularExpression("\\b0[xX][0-9a-fA-F]+\\b"),  // 十六进制
        numberFormat
    });
    highlightingRules.append({
        QRegularExpression("\\b0[bB][01]+\\b"),          // 二进制
        numberFormat
    });
    highlightingRules.append({
        QRegularExpression("\\b\\d+(\\.\\d+)?([eE][+-]?\\d+)?[fFlL]?\\b"),  // 整数/浮点
        numberFormat
    });

    // ============ 4. 字符串 (橙色) ============
    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#CE9178"));

    highlightingRules.append({
        QRegularExpression(R"("(?:[^"\\]|\\.)*")"),      // 双引号字符串
        stringFormat
    });
    highlightingRules.append({
        QRegularExpression(R"('(?:[^'\\]|\\.)')"),       // 单引号字符
        stringFormat
    });
    // 原始字符串 R"(...)"
    highlightingRules.append({
        QRegularExpression(R"(R"([^()]*)\((.*?)\)\1")"),
        stringFormat
    });

    // ============ 5. 转义字符 (金色) ============
    QTextCharFormat escapeFormat;
    escapeFormat.setForeground(QColor("#D7BA7D"));

    highlightingRules.append({
        QRegularExpression("\\\\[nrt\\\\\"\'abfnrtv0]|\\\\x[0-9a-fA-F]{2}|\\\\u[0-9a-fA-F]{4}"),
        escapeFormat
    });

    // ============ 6. 预处理器指令 (紫色) ============
    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(QColor("#C586C0"));

    highlightingRules.append({
        QRegularExpression("^\\s*#\\s*(include|define|undef|if|ifdef|ifndef|else|elif|endif|pragma|error|warning|line)\\b"),
        preprocessorFormat
    });

    // 头文件名 (紫色稍浅)
    QTextCharFormat headerFormat;
    headerFormat.setForeground(QColor("#CE89A8"));

    highlightingRules.append({
        QRegularExpression("[<\"][a-zA-Z0-9_/]+\\.h[pp]?[\">]"),
        headerFormat
    });

    // ============ 7. 单行注释 (绿色) ============
    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(QColor("#6A9955"));
    singleLineCommentFormat.setFontItalic(true);

    highlightingRules.append({
        QRegularExpression("//[^\n]*"),
        singleLineCommentFormat
    });

    // ============ 8. 多行注释 (绿色) ============
    multiLineCommentFormat.setForeground(QColor("#6A9955"));
    multiLineCommentFormat.setFontItalic(true);

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");

    // ============ 9. 函数名 (黄色) ============
    QTextCharFormat functionFormat;
    functionFormat.setForeground(QColor("#DCDCAA"));

    highlightingRules.append({
        QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()"),
        functionFormat
    });

    // ============ 10. 类/类型名 (青绿色) — 大写开头的标识符 ============
    QTextCharFormat classFormat;
    classFormat.setForeground(QColor("#4EC9B0"));

    highlightingRules.append({
        QRegularExpression("\\b[A-Z][A-Za-z0-9_]*\\b"),
        classFormat
    });

    // ============ 11. 成员访问 (点号后) — 黄色 ============
    QTextCharFormat memberFormat;
    memberFormat.setForeground(QColor("#DCDCAA"));

    highlightingRules.append({
        QRegularExpression("(?<=\\.|->)[A-Za-z_][A-Za-z0-9_]*"),
        memberFormat
    });
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // 先处理多行注释
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
    {
        QRegularExpressionMatch startMatch = commentStartExpression.match(text);
        startIndex = startMatch.hasMatch() ? startMatch.capturedStart() : -1;
    }

    while (startIndex >= 0)
    {
        QRegularExpressionMatch endMatch = commentEndExpression.match(text, startIndex + 2);
        int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;

        int commentLength;
        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);

        QRegularExpressionMatch nextStartMatch = commentStartExpression.match(text, startIndex + commentLength);
        startIndex = nextStartMatch.hasMatch() ? nextStartMatch.capturedStart() : -1;
    }

    // 然后处理单行规则
    for (const HighlightingRule &rule : highlightingRules)
    {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
