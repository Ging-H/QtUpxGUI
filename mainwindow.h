#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QThread>
#include "multiupxmanager.h"

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    enum COMPRESSION_LEVELS{
        Default = 0,
        Lv1,   // 速度最快
        Lv2,
        Lv3,
        Lv4,
        Lv5,
        Lv6,
        Lv7,
        Lv8,   // default
        Lv9,   // 效率较高
        Best,  // 效率最高
    };
    struct UpxOption{
        bool isCompress;   // 压缩或解压
        bool isBackupFile;
        COMPRESSION_LEVELS levels;

    };


    QStandardItem *getFileTree(QString root);
    QIcon extractIcon(QString path);
    void treeSetAllChildItemState( QStandardItem *parent);
    void treeSetParentItemState(QStandardItem *item);
    QStringList treeGetFilePath( QStandardItem *item );
    void runUpx( QStringList fileName, UpxOption option);
    QStandardItem *treeGetItemFormText( QStandardItem *parent, QString text );
    void createFileMap( QStandardItemModel* model);
    void resetFileStatus();


signals :
    void signals_compress(QStringList);

private slots:
    void on_btnBrowser_clicked();

    void slots_treeItemChange(QStandardItem *item);

    void on_btnCpsSlt_clicked();

    void on_cbbSpeed_currentIndexChanged(int index);

    void on_cbbBackup_currentIndexChanged(int index);

    void slots_updateFileStatus( QString file, MultiUpxManager::FileStatus);

    void on_btnClearLst_clicked();

    void on_btnDecompress_clicked();

    void on_btnDeleteBackup_clicked();

    void on_btnRecover_clicked();

    void slots_treeViewMenuRequested(const QPoint &pos);

    void on_actionTest_triggered();

    void on_actionDelete_triggered();

    void on_actionDeleteFile_triggered();

    void slots_LogMsg(QString msg);
    void on_actionClearTxt_triggered();

private:

    Ui::MainWindow *ui;
    QDir execDir;
    quint32 fileCount = 0;
    quint32 fileChecked = 0;
    UpxOption option = {true, true, Default};
    QThread thread;
    QMap<QString, QStandardItem *> *fileStatusMap = NULL;// 文件映射
};

#endif // MAINWINDOW_H
