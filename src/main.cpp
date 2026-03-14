#include <QApplication>
#include <QDateTime>
#include <QFont>
#include <QFontInfo>

#include "mainwindow.h"

static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    const QString ts  = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QByteArray out = QString("[%1] %2").arg(ts, msg).toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:    fprintf(stderr, "%s\n",          out.constData()); break;
    case QtWarningMsg: fprintf(stderr, "Warning: %s\n", out.constData()); break;
    case QtCriticalMsg:fprintf(stderr, "Critical: %s\n",out.constData()); break;
    case QtFatalMsg:   fprintf(stderr, "Fatal: %s\n",   out.constData()); abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageHandler);
    QApplication app(argc, argv);

    QFont appFont("Segoe UI");
    if (!QFontInfo(appFont).family().contains("Segoe UI", Qt::CaseInsensitive)) {
        appFont = QApplication::font();
    }
    appFont.setPointSize(9);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    MainWindow window;
    window.show();
    return app.exec();
}
