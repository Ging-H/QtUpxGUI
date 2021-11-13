#include "multiupxmanager.h"
#include "upxui.h"
#include <QMap>
#include <QDebug>
#include <QProcess>

MultiUpxManager::MultiUpxManager()
{
    /* default value */
    argLv.clear();
    argBuckup = "-k";
}
/**
 * @brief MultiUpxManager::setBackupEnable 设定备份文件
 * @param enable
 */
void MultiUpxManager::setBackupEnable(bool enable)
{
    argBuckup.clear();
    if( enable ){
        argBuckup = "-k";
    }
}
/**
 * @brief MultiUpxManager::setCompressionLv 设定压缩级别
 * @param value  压缩级别
 */
void MultiUpxManager::setCompressionLv(const quint32 &value)
{
    argLv.clear();
    switch( value ){
    case 10: argLv = "--best";
        break;
    default:
        if( (value <= 9) && (value >= 1) )
            argLv = "-" + QString::number(value);
        break;
    }
}

/**
 * @brief MultiUpxManager::compress 压缩文件
 * @param filePath
 */
void MultiUpxManager::compress(QStringList filePath , qint32 multiThread)
{
    createMap(filePath);
    fileCount = filePath.count();
    upx.clear();
    if( fileCount < multiThread ){ // 创建多个线程
        multiThread = fileCount;
    }
    while(multiThread -- ){
        upx<<new QProcess();
    }

    foreach(auto process, upx){
        connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus){
            Q_UNUSED(exitStatus)
            QString file = process->arguments().last();
            switch( exitCode ){
            case  0:
                fileStaMap[file] = Success;
                emit signals_fileStatus(file, Success);     // 压缩成功
                break;
            case  1:
                fileStaMap[file] = FileNotFound;
                emit signals_fileStatus(file, FileNotFound); // 未找到文件
                break;
            case  2:
                fileStaMap[file] = HasCompress;
                emit signals_fileStatus(file, HasCompress); // 压缩失败(已经被压缩)
                break;
            default:
                fileStaMap[file] = Error;
                emit signals_fileStatus(file, Error);       // 压缩失败(未知错误)
                qDebug()<<"file err:"<<file;
                qDebug()<<"exitCode:"<<exitCode;
                break;
            }
            file = process->readAllStandardError();
            if( !file.isEmpty() )
                emit signals_errMsg(file);

            fileCount--;
            if(fileCount <= 0){
                emit signals_finishCompress();
            }

            /* next file */
            QString fileName = getIdleFile();
            if( fileName.isEmpty() ){
                return;
            }
            QStringList arg;
            if( !argBuckup.isEmpty() ){
                arg <<  argBuckup ;
            }
            if( !argLv.isEmpty() ){
                arg << argLv;
            }
            arg << fileName;
            process->start( "upx.exe", arg );
            emit signals_fileStatus(fileName, Compressing);// 启动信号
        });

        /* 启动多个线程 */
        QString fileName = getIdleFile();
        if( fileName.isEmpty() ){
            return;
        }
        QStringList arg;
        if( !argBuckup.isEmpty() ){
            arg <<  argBuckup ;
        }
        if( !argLv.isEmpty() ){
            arg << argLv;
        }
        arg << fileName;
        process->start( "upx.exe", arg );
        emit signals_fileStatus(fileName, Compressing);
    }
}

/**
 * @brief MultiUpxManager::createMap 建立文件与状态量的映射
 * @param fIlePath
 */
void MultiUpxManager::createMap( QStringList &filePath )
{
    fileStaMap.clear();
    foreach (auto path, filePath ) {
        fileStaMap[path] = Idle;
    }
}
/**
 * @brief MultiUpxManager::getIdleFile 获取未压缩的文件路径,并且标记未压缩中
 * @return
 */
QString MultiUpxManager::getIdleFile()
{
    if( fileStaMap.isEmpty() ){
        return QString();
    }

    QList<QString> keys = fileStaMap.keys( Idle );
    if( keys.empty() ){
        return QString();
    }else{
        fileStaMap[keys.at(0)] = Compressing;
        return keys.at(0);
    }
}
/**
 * @brief MultiUpxManager::compress 压缩文件
 * @param filePath
 */
void MultiUpxManager::decompress(QStringList filePath , qint32 multiThread)
{
    createMap(filePath);
    fileCount = filePath.count();
    upx.clear();
    if( fileCount < multiThread ){ // 创建多个线程
        multiThread = fileCount;
    }
    while(multiThread -- ){
        upx<<new QProcess();
    }

    foreach(auto process, upx){
        connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus){
            Q_UNUSED(exitStatus)
            QString file = process->arguments().last();
            switch( exitCode ){
            case  0:
                fileStaMap[file] = Success;
                emit signals_fileStatus(file, Success);      // 解压成功
                break;
            case  1:
                fileStaMap[file] = FileNotFound;
                emit signals_fileStatus(file, FileNotFound); // 未找到文件
                break;
            case  2:
                fileStaMap[file] = NotCompress;
                emit signals_fileStatus(file, NotCompress);  // 压缩失败(已经被压缩)
                break;
            default:
                fileStaMap[file] = Error;
                emit signals_fileStatus(file, Error);       // 压缩失败(未知错误)
                qDebug()<<"file err:"<<file;
                qDebug()<<"exitCode:"<<exitCode;
                break;
            }
            file = process->readAllStandardError();
            if( !file.isEmpty() )
                emit signals_errMsg(file);

            fileCount--;
            if(fileCount <= 0){
                emit signals_finishCompress();
            }

            /* next file */
            QString fileName = getIdleFile();
            if( fileName.isEmpty() ){
                return;
            }
            QStringList arg;
            arg<<"-d";       // 解压
            arg << fileName;
            process->start( "upx.exe", arg );
            emit signals_fileStatus(fileName, Decompressing);// 启动
        });

        /* 启动多个线程 */
        QString fileName = getIdleFile();
        if( fileName.isEmpty() ){
            return;
        }
        QStringList arg;
        arg<<"-d";           // 解压
        arg << fileName;
        process->start( "upx.exe", arg );
        emit signals_fileStatus(fileName, Decompressing);
    }
}
/**
 * @brief MultiUpxManager::test
 * @param filePath
 */
void MultiUpxManager::test(QString filePath )
{
    QProcess *upx_ = new QProcess();
    QStringList arg;
    arg<<"-t"; // 测试
    arg<<filePath;
    upx_->start( "upx.exe", arg);
    upx_->waitForStarted(100);
    upx_->waitForFinished(100);
    filePath = upx_->readAllStandardError();
    emit signals_errMsg(filePath);
    delete upx_;
}

