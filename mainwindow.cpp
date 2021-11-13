#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWindow>
#include <QFileIconProvider>
#include <QTime>
#include <QMessageBox>

#if _MSC_VER >=1600 //VS2010版本号是1600
#pragma execution_character_set("utf-8")
#endif

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QStandardItemModel* model = new QStandardItemModel(ui->lstFileList);
    ui->lstFileList->setModel( model );
    model->setHorizontalHeaderLabels( QStringList()<<QStringLiteral( "file") << QStringLiteral("status")); //
    ui->lstFileList->setModel(model);

    ui->lstFileList->setColumnWidth(0, 350);
    ui->lstFileList->setColumnWidth(1, 100);
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 3);
    ui->splitter_2->setStretchFactor(0, 3);
    ui->splitter_2->setStretchFactor(1, 2);

    fileStatusMap = new QMap<QString, QStandardItem *>;
    connect(ui->lstFileList, &QTreeView::customContextMenuRequested, this, &MainWindow::slots_treeViewMenuRequested);
    connect(ui->txtLog, &QPlainTextEdit::customContextMenuRequested, [=](){
        QMenu menu;
        menu.addAction(ui->actionClearTxt);
        menu.exec(QCursor::pos());
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief MainWindow::on_btnBrowser_clicked 浏览文件夹
 */
void MainWindow::on_btnBrowser_clicked()
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filepath = QFileDialog::getExistingDirectory(this, QString(), desktop);
    if( filepath.endsWith('/') || filepath.endsWith('\\')  ){
        return;
    }
    if( filepath.isEmpty() ){
        return;
    }
    fileCount = 0;
    fileStatusMap->clear();
    QStandardItem *item = getFileTree(filepath);
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    for(qint32 i=0; i<model->rowCount();i++){
        qDeleteAll(model->takeRow(0));
    }
    model->appendRow(item);
    connect(model, &QStandardItemModel::itemChanged,
            this, &MainWindow::slots_treeItemChange );
    ui->lstFileList->expandAll();
}


/**
 * @brief MainWindow::getFileTree
 * @param root
 * @return
 */
QStandardItem *MainWindow::getFileTree(QString root)
{
    execDir.setPath( root );
    execDir.setFilter( QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot );
    execDir.setSorting( QDir::DirsFirst | QDir::Type | QDir::Name );

    QFileInfoList infolist = execDir.entryInfoList();
    QStandardItem *rootItem = new QStandardItem();
    foreach (auto info, infolist) {
        QStandardItem *item;

        if( info.isDir() ){
            item = getFileTree( info.filePath() );// 二级节点,子文件夹
            item->setText( info.fileName() );     // 以文件夹命名二级节点
            item->setCheckable(true);
            item->setCheckState(Qt::Checked);
            item->setIcon( extractIcon( info.filePath() ) );
            rootItem->appendRow(item);
        }else{
            item = new QStandardItem();
            item->setText( info.fileName() ); // 以文件名命名一级节点
            item->setData( info.filePath());  // 记录路径
            fileCount++;
            item->setCheckable(true);
            item->setCheckState(Qt::Checked);
            item->setIcon( extractIcon( info.filePath() ) );
            QStandardItem *statusItem = new QStandardItem();
            statusItem->setText("idle");
            statusItem->setData( info.filePath() );
            fileStatusMap->insert( info.filePath(), statusItem );// 将文件状态列跟路径建立映射
            QList< QStandardItem *> items;
            items.append(item);
            items.append(statusItem);
            rootItem->appendRow(items);
        }
    }
    rootItem->setText( root );
    rootItem->setCheckable(true);
    return rootItem;
}

/**
 * @brief MainWindow::extracticon 解压图标
 * @param path 文件路径
 */
QIcon MainWindow::extractIcon(QString path){
    QFileInfo file(path);
    QFileIconProvider seekIcon;
    QIcon icon = seekIcon.icon( file );
    return icon;
}

/**
 * @brief slots_treeItemChange item复选框勾选之后的动作
 * @param item
 */
void MainWindow::slots_treeItemChange(QStandardItem *item)
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    model->disconnect( SIGNAL(itemChanged(QStandardItem*)) );

    if( item->hasChildren() ){
        treeSetAllChildItemState(item );

    }
    /* 同时修改父项状态 */
    if( item->parent() != NULL ){
        treeSetParentItemState( item->parent() );
    }
    connect(model, &QStandardItemModel::itemChanged,
            this, &MainWindow::slots_treeItemChange);
}
/**
 * @brief MainWindow::treeSetParentItemState 设置父项的状态
 * @param item 父项
 */
void MainWindow::treeSetParentItemState(QStandardItem *parent)
{
    quint32 count = parent->rowCount();
    qint32 uncheckCnt = 0;
    if( count == 0 )
        return;
    for(quint32 i=0; i<count; i++){
        QStandardItem *subItem = parent->child(i);
        if( subItem->checkState() == Qt::Unchecked ){
            uncheckCnt++;
        }else if(subItem->checkState() == Qt::Checked) {
            uncheckCnt--;
        }
    }
    if( uncheckCnt == count ){
        parent->setCheckState( Qt::Unchecked );
    }else if( abs(uncheckCnt) == count) {
        parent->setCheckState( Qt::Checked );
    }else{
        parent->setCheckState( Qt::PartiallyChecked );
    }
    if(parent->parent() != NULL)
        treeSetParentItemState(parent->parent());
}
/**
 * @brief MainWindow::treeChildItemChange 设置所有子项的状态
 * @param checkState 两态
 */
void MainWindow::treeSetAllChildItemState( QStandardItem *parent )
{
    Qt::CheckState checked = parent->checkState();
    quint32 count = parent->rowCount();
    for( quint32 i=0; i<count; i++ ){
        QStandardItem *subItem = parent->child(i);
        subItem->setCheckState( checked );
        if( subItem->hasChildren() ){
            treeSetAllChildItemState(subItem);
        }
    }
}
/**
 * @brief MainWindow::on_btnCpsSlt_clicked 压缩选择的文件
 */
void MainWindow::on_btnCpsSlt_clicked()
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    quint32 count = model->rowCount();
    if( count == 0 ){
        return;
    }
    QStringList filePath = treeGetFilePath( model->item(0) );
    if( filePath.isEmpty() )
        return;
    fileChecked = filePath.count();
    ui->pgsGlobal->setMaximum( fileChecked );
    ui->pgsGlobal->setValue(0);
    resetFileStatus();
    option.isCompress = true;
    runUpx(filePath, option);
}
/**
 * @brief MainWindow::reflashStatus 复位文件状态
 */
void MainWindow::resetFileStatus()
{
    QMap<QString, QStandardItem *>::iterator  iter = fileStatusMap->begin();
    while( iter != fileStatusMap->end() ){
        iter.value()->setText("idle");
        iter++;
    }
}

/**
 * @brief MainWindow::getFilePath 获取列表中的文件路径
 */
QStringList MainWindow::treeGetFilePath( QStandardItem *item )
{
    QStringList path;
    quint32 count = item->rowCount();
    for( quint32 i=0; i<count; i++ ){
        QStandardItem *subItem = item->child(i);
        if( subItem->hasChildren() ){
            path.append( treeGetFilePath(subItem) );
        }else{
            if( subItem->checkState() != Qt::Checked ){
                continue;
            }
            QString filepath = subItem->data().toString();
            path.append( filepath );
        }
    }
    return path;
}

/**
 * @brief MainWindow::runUpx --> 执行压缩程序
 * @param fileName           --> 文件路径清单
 */
void MainWindow::runUpx( QStringList fileName, UpxOption option )
{
    MultiUpxManager *upxManager = new MultiUpxManager;
    upxManager->setBackupEnable( option.isBackupFile );
    upxManager->setCompressionLv( option.levels );
    if( option.isCompress ){
        upxManager->compress( fileName, ui->spbMultiThread->value() );
    }else{
        upxManager->decompress( fileName, ui->spbMultiThread->value() );
    }
    connect( upxManager,&MultiUpxManager::signals_fileStatus,
             this, &MainWindow::slots_updateFileStatus);
    connect( upxManager, &MultiUpxManager::signals_finishCompress,
             upxManager, &MultiUpxManager::deleteLater);
    connect( upxManager, &MultiUpxManager::signals_errMsg,
             this, &MainWindow::slots_LogMsg);

}

/**
 * @brief MainWindow::on_cbbSpeed_currentIndexChanged 设定压缩级别/速度
 * @param index
 */
void MainWindow::on_cbbSpeed_currentIndexChanged(int index)
{
    option.levels = (COMPRESSION_LEVELS)index;
}
/**
 * @brief MainWindow::on_cbbBackup_currentIndexChanged 选择是否保留备份文件
 * @param index
 */
void MainWindow::on_cbbBackup_currentIndexChanged(int index)
{
    if( index == 0 ){
        option.isBackupFile = true;
    }else{
        option.isBackupFile = false;
    }
}


/**
 * @brief slots_updateFileStatus 更新文件状态
 * @param file 文件
 * @param MultiUpxManager::FileStatus 状态
 */
void MainWindow::slots_updateFileStatus( QString file, MultiUpxManager::FileStatus status)
{
    QStandardItem *staItem = fileStatusMap->value( file );
    if( staItem == NULL ){
        return;
    }
    quint32 val = ui->pgsGlobal->value();

    /* 显示压缩状态 */
    switch( status ){
    case  MultiUpxManager::Success:
        staItem->setText( tr("Success") );
        val++;
        break;
    case  MultiUpxManager::Compressing:
        staItem->setText( tr("Compressing") );
        break;
    case  MultiUpxManager::Error:
        staItem->setText( tr("Err:Unknow") );
        val++;
        break;
    case  MultiUpxManager::HasCompress:
        staItem->setText( tr("Err:HasCompress") );
        val++;
        break;
    case  MultiUpxManager::FileNotFound:
        staItem->setText( tr("Err:NotFoundOrSupport") );
        val++;
        break;
    case  MultiUpxManager::NotCompress:
        staItem->setText( tr("Err:NotCompress") );
        val++;
        break;
    default:
        break;
    }
    ui->pgsGlobal->setValue(val);
}
/**
 * @brief MainWindow::on_btnClearLst_clicked 清空列表
 */
void MainWindow::on_btnClearLst_clicked()
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    for(qint32 i=0; i<model->rowCount();i++){
        qDeleteAll(model->takeRow(0));
    }
    ui->txtLog->clear();
}
/**
 * @brief MainWindow::on_btnRecover_clicked 解压所选文件
 */
void MainWindow::on_btnDecompress_clicked()
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    quint32 count = model->rowCount();
    if( count == 0 ){
        return;
    }
    QStringList filePath = treeGetFilePath( model->item(0) );
    if( filePath.isEmpty() )
        return;
    fileChecked = filePath.count();
    ui->pgsGlobal->setMaximum( fileChecked );
    ui->pgsGlobal->setValue(0);
    resetFileStatus();
    option.isCompress = false;
    runUpx(filePath, option);
}
/**
 * @brief MainWindow::on_pushButton_clicked 删除所选文件的备份文件
 */
void MainWindow::on_btnDeleteBackup_clicked()
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    quint32 count = model->rowCount();
    if( count == 0 ){
        return;
    }
    QStringList filePath = treeGetFilePath( model->item(0) );
    if( filePath.isEmpty() )
        return;
    foreach (QString path, filePath) {
        path.chop(1);
        path.append('~');
        QFile::remove(path);
    }
}
/**
 * @brief MainWindow::on_btnRecover_clicked 恢复备份文件
 */
void MainWindow::on_btnRecover_clicked()
{
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    quint32 count = model->rowCount();
    if( count == 0 ){
        return;
    }
    QStringList filePath = treeGetFilePath( model->item(0) );
    if( filePath.isEmpty() )
        return;
    foreach (QString backup, filePath) {
        QString path = backup;
        backup.chop(1);
        backup.append('~');
        QFileInfo info( backup );
        if( info.isFile() ){
            QFile::remove(path);
            QFile file(backup);
            file.rename( path );
        }
    }
}

/**
 * @brief slots_treeViewMenuRequested 列表的右键菜单:测试应用
 * @param pos
 */
void MainWindow::slots_treeViewMenuRequested(const QPoint &pos)
{
    QModelIndex curIndex = ui->lstFileList->indexAt(pos);
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    if (curIndex.isValid()) // 右键选中了有效index
    {
        // 创建菜单
        QMenu menu;
        if(model ->itemFromIndex(curIndex)->text().endsWith(".exe")){
            menu.addAction(ui->actionTest);

        }
        menu.addAction(ui->actionDelete);
        menu.addAction(ui->actionDeleteFile);
        menu.exec(QCursor::pos());
    }
}
/**
 * @brief MainWindow::on_actionTest_triggered 测试应用
 */
void MainWindow::on_actionTest_triggered()
{
    QModelIndex curIndex = ui->lstFileList->currentIndex();
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    QString path = model->itemFromIndex(curIndex)->data().toString();
    MultiUpxManager *upxManager = new MultiUpxManager;
    upxManager->test(path);
}
/**
 * @brief MainWindow::on_actionDelete_triggered 删除文件
 */
void MainWindow::on_actionDelete_triggered()
{
    QModelIndex curIndex = ui->lstFileList->currentIndex();
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    QStandardItem *parent = model->itemFromIndex(curIndex)->parent();
    parent->removeRow( curIndex.row() );
    while( (!parent->hasChildren()) ){
        QModelIndex curIndex = model->indexFromItem(parent);
        parent = parent->parent();
        if( parent == NULL ){
            model->removeRow(0);
            break;
        }
        parent->removeRow( curIndex.row() );
    }
}

/**
 * @brief MainWindow::on_actionDeleteFile_triggered 删除源文件
 */
void MainWindow::on_actionDeleteFile_triggered()
{
    if ( QMessageBox::warning(this, "warning", "will delete file form disk,\n\tsure?",
                              QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Cancel)
        return;
    QModelIndex curIndex = ui->lstFileList->currentIndex();
    QStandardItemModel* model = (QStandardItemModel*)ui->lstFileList->model();
    QStandardItem *parent  = model->itemFromIndex(curIndex)->parent();
    QStandardItem *curItem = model->itemFromIndex(curIndex);
    QFile::remove( (curItem->data().toString()) );
    parent->removeRow( curIndex.row() );
    while( (!parent->hasChildren()) ){
        QModelIndex curIndex = model->indexFromItem(parent);
        parent = parent->parent();
        if( parent == NULL ){
            model->removeRow(0);
            break;
        }
        parent->removeRow( curIndex.row() );
    }
}
/**
 * @brief MainWindow::slots_LogMsg 记录信息
 */
void MainWindow::slots_LogMsg(QString msg)
{
    msg = QTime::currentTime().toString("[hh-mm-ss.zzz]: ") + msg;
    ui->txtLog->appendPlainText( msg );
}

/**
 * @brief MainWindow::on_actionClearTxt_triggered 清空错误信息
 */
void MainWindow::on_actionClearTxt_triggered()
{
    ui->txtLog->clear();
}
