#include "AgentManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QDebug>

AgentManager::AgentManager(const QString &projectPath, const QString &apiKey, QObject *parent)
    : QObject(parent), projectPath(projectPath), apiKey(apiKey), running(false), restartPending(false)
{
    agentProcess = new QProcess(this);
    setupProcess();
}

AgentManager::~AgentManager()
{
    stop();
}

void AgentManager::setupProcess()
{
    connect(agentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AgentManager::onProcessFinished);
    connect(agentProcess, &QProcess::readyReadStandardOutput,
            this, &AgentManager::onReadyReadStandardOutput);
    connect(agentProcess, &QProcess::readyReadStandardError,
            this, &AgentManager::onReadyReadStandardError);
    connect(agentProcess, &QProcess::errorOccurred,
            this, &AgentManager::onProcessError);
}

void AgentManager::startProcess()
{
    // 设置工作目录
    agentProcess->setWorkingDirectory(projectPath);
    qDebug() << "设置工作目录:" << projectPath;

    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!apiKey.isEmpty())
    {
        env.insert("DEEPSEEK_API_KEY", apiKey);
        qDebug() << "设置环境变量 DEEPSEEK_API_KEY";
    }
    else
    {
        qDebug() << "API Key 为空，不设置环境变量";
    }
    env.insert("YCODE_MANAGED", "1");
    env.insert("YCODE_PROJECT_ROOT", QDir::toNativeSeparators(projectPath));
    agentProcess->setProcessEnvironment(env);

    // 查找 agent.exe
    QString agentPath = projectPath + "/agent.exe";
    qDebug() << "查找 agent.exe:" << agentPath;
    if (!QFile::exists(agentPath))
    {
        QString errorMsg = "找不到 agent.exe 文件: " + agentPath;
        qDebug() << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }

    qDebug() << "agent.exe 存在，开始启动...";
    // 启动进程
    agentProcess->start(agentPath, QStringList());
    if (!agentProcess->waitForStarted(5000))
    {
        QString errorMsg = "启动 agent.exe 失败: " + agentProcess->errorString();
        qDebug() << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }

    running = true;
    restartPending = false;
    qDebug() << "Agent 启动成功";
    emit statusChanged("Agent 已启动");
}

void AgentManager::start()
{
    qDebug() << "AgentManager::start() 被调用";
    qDebug() << "running:" << running;
    qDebug() << "projectPath:" << projectPath;
    qDebug() << "apiKey 长度:" << apiKey.length();

    if (running)
        return;

    startProcess();
}

void AgentManager::stop()
{
    if (!running)
        return;

    agentProcess->terminate();
    if (!agentProcess->waitForFinished(3000))
    {
        agentProcess->kill();
        agentProcess->waitForFinished(1000);
    }

    running = false;
    restartPending = false;
    emit statusChanged("Agent 已停止");
}

// ★ 新增：主动重启 agent.exe
void AgentManager::restart()
{
    qDebug() << "AgentManager::restart() 被调用";
    emit agentRestarting();
    emit statusChanged("Agent 正在重启...");

    // 先停止旧进程
    if (running)
    {
        agentProcess->terminate();
        if (!agentProcess->waitForFinished(3000))
        {
            agentProcess->kill();
            agentProcess->waitForFinished(1000);
        }
        running = false;
    }

    restartPending = true;

    // 稍等片刻后启动新进程（给系统时间释放资源）
    QTimer::singleShot(500, this, [this]()
    {
        qDebug() << "延迟后启动新 Agent 进程...";
        startProcess();
    });
}

bool AgentManager::isRunning() const
{
    return running && agentProcess->state() == QProcess::Running;
}

void AgentManager::sendMessage(const QString &message)
{
    if (!isRunning())
    {
        emit errorOccurred("Agent 未运行");
        return;
    }

    // 发送消息到 agent.exe 的标准输入
    QByteArray data = (message + "\n").toUtf8();
    agentProcess->write(data);
    agentProcess->waitForBytesWritten(1000);
}

void AgentManager::onReadyReadStandardOutput()
{
    QByteArray data = agentProcess->readAllStandardOutput();
    QString output = QString::fromUtf8(data).trimmed();

    if (!output.isEmpty())
    {
        const QString restartAgentSignal = "SIGNAL:RESTART_AGENT";
        const QString rebuildRestartYCodeSignal = "SIGNAL:REBUILD_RESTART_YCODE";
        const QString reloadStyleSignal = "SIGNAL:RELOAD_STYLE";

        if (output.contains(rebuildRestartYCodeSignal))
        {
            qDebug() << "检测到 YCode 自更新信号:" << rebuildRestartYCodeSignal;

            QString userOutput = output;
            userOutput.replace(rebuildRestartYCodeSignal, "");
            userOutput = userOutput.trimmed();
            if (!userOutput.isEmpty())
            {
                emit outputReceived(userOutput);
            }

            emit ycodeSelfUpdateRequested();
            return;
        }

        if (output.contains(reloadStyleSignal))
        {
            qDebug() << "检测到样式热加载信号:" << reloadStyleSignal;

            QString userOutput = output;
            userOutput.replace(reloadStyleSignal, "");
            userOutput = userOutput.trimmed();
            if (!userOutput.isEmpty())
            {
                emit outputReceived(userOutput);
            }

            emit reloadStyleRequested();
            return;
        }

        // ★ 检测重启信号
        if (output.contains(restartAgentSignal))
        {
            qDebug() << "检测到 Agent 重启信号:" << restartAgentSignal;
            // 去掉信号标记，把剩余内容显示给用户
            QString userOutput = output;
            userOutput.replace(restartAgentSignal, "");
            userOutput = userOutput.trimmed();
            if (!userOutput.isEmpty())
            {
                emit outputReceived(userOutput);
            }

            // 触发重启（由 YCode 客户端接管）
            restart();
            return;
        }

        emit outputReceived(output);
    }
}

void AgentManager::onReadyReadStandardError()
{
    QByteArray data = agentProcess->readAllStandardError();
    QString error = QString::fromUtf8(data).trimmed();

    if (!error.isEmpty())
    {
        emit errorOccurred(error);
    }
}

void AgentManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Agent 进程结束, exitCode:" << exitCode;

    // ★ 如果是有计划的重启（restartPending），不需要报错
    if (restartPending)
    {
        qDebug() << "Agent 正在计划内重启，不报告错误...";
        // restartPending 会在 startProcess() 中重置
        return;
    }

    running = false;

    if (exitStatus == QProcess::CrashExit)
    {
        emit errorOccurred("Agent 进程崩溃");
    }
    else if (exitCode != 0)
    {
        emit errorOccurred(QString("Agent 退出，代码: %1").arg(exitCode));
    }

    emit statusChanged("Agent 已退出");
}

void AgentManager::onProcessError(QProcess::ProcessError error)
{
    // ★ 如果是有计划的重启，忽略进程错误
    if (restartPending)
    {
        qDebug() << "重启中，忽略进程错误:" << error;
        return;
    }

    running = false;

    QString errorMsg;
    switch (error)
    {
    case QProcess::FailedToStart:
        errorMsg = "启动失败";
        break;
    case QProcess::Crashed:
        errorMsg = "进程崩溃";
        break;
    case QProcess::Timedout:
        errorMsg = "超时";
        break;
    case QProcess::WriteError:
        errorMsg = "写入错误";
        break;
    case QProcess::ReadError:
        errorMsg = "读取错误";
        break;
    default:
        errorMsg = "未知错误";
        break;
    }

    emit errorOccurred("Agent 错误: " + errorMsg);
    emit statusChanged("Agent 错误");
}
