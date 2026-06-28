#ifndef AGENTMANAGER_H
#define AGENTMANAGER_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

class AgentManager : public QObject
{
    Q_OBJECT

public:
    explicit AgentManager(const QString &projectPath, const QString &apiKey, QObject *parent = nullptr);
    ~AgentManager();

    void start();
    void stop();
    void restart();                // ★ 新增：由 YCode 主动重启 agent 进程
    bool isRunning() const;
    void sendMessage(const QString &message);
    void setWorkspacePath(const QString &path);

signals:
    void outputReceived(const QString &output);
    void errorOccurred(const QString &error);
    void statusChanged(const QString &status);
    void agentRestarting();        // ★ 新增：agent 正在重启信号
    void ycodeSelfUpdateRequested();
    void reloadStyleRequested();

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void setupProcess();
    void startProcess();           // ★ 抽取启动逻辑

    QProcess *agentProcess;
    QString projectPath;
    QString workspacePath;
    QString apiKey;
    bool running;
    bool restartPending;           // ★ 标记是否有待处理的重启
};

#endif
