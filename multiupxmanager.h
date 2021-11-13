#ifndef MULTITHREADMANAGER_H
#define MULTITHREADMANAGER_H

#include <QObject>
#include <Qmap>
#include <QProcess>
class MultiUpxManager :public QObject
{
    Q_OBJECT
public:
    MultiUpxManager();

    enum FileStatus {
        Idle,          // 空闲状态,未压缩
        Success,       // 压缩成功(完成)
        Compressing,   // 压缩中
        Decompressing, // 解压中
        Error,         // 压缩失败(完成)
        HasCompress,   // 压缩失败(已经被压缩)
        FileNotFound,  // 压缩失败(已经被压缩)
        NotCompress,   // 解压失败(没有被压缩)
    };

    void setBackupEnable(bool enable);
    void setCompressionLv(const quint32 &value);
    void createMap( QStringList &fIlePath );
    QString getIdleFile();
    void compress(QStringList filePath , qint32 multiThread);
    void decompress(QStringList filePath , qint32 multiThread);
    void test(QString filePath );


signals:
    void signals_fileStatus( QString , FileStatus );
    void signals_finishCompress( );
    void signals_errMsg( QString );

public slots:



private:
    QString  argLv;
    QString  argBuckup;
    QMap<QString, FileStatus> fileStaMap;
    qint32 fileCount = 0;
    QList< QProcess *> upx;
};

#endif // MULTITHREADMANAGER_H
