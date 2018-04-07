#include <QUrl>
#include <QFile>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>
#include <QMimeData>
#include <QMessageBox>
#include <QStringListModel>
#include <QFileDialog>
#include <QList>
#include <QImage>
#include <QImageWriter>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include <cmath>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_pUi(new Ui::MainWindow)
{
    m_pUi->setupUi(this);

    setAcceptDrops(true);

    loadSettings();

    m_pFileListModel = new QStringListModel();
    m_pUi->listView->setModel(m_pFileListModel);

    m_pFutureWatcherList = new QFutureWatcherList();
    connect(m_pFutureWatcherList, SIGNAL(futures_finished()), this, SLOT(futures_finished()));

    connect(m_pFileListModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
            this, SLOT(fileListModel_dataChanged()));

    connect(m_pUi->listView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(listView_selectionModel_update()));

    fileListModel_dataChanged();
    listView_selectionModel_update();
}

MainWindow::~MainWindow()
{
    delete m_pUi;
    delete m_pFileListModel;
    delete m_pFutureWatcherList;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    if (!e->mimeData()->hasUrls())
    {
        return;
    }

    e->acceptProposedAction();

    foreach (const QUrl &url, e->mimeData()->urls())
    {
        QString fileName = url.toLocalFile();
        if (fileName.endsWith(".png", Qt::CaseInsensitive) ||
            fileName.endsWith(".jpg", Qt::CaseInsensitive) ||
            fileName.endsWith(".jpeg", Qt::CaseInsensitive))
        {
            int rowCount = m_pFileListModel->rowCount();
            m_pFileListModel->insertRow(rowCount);
            m_pFileListModel->setData(m_pFileListModel->index(rowCount), fileName);
        }
    }

    if (m_pUi->autoConvert_checkBox->isChecked() && m_pFileListModel->rowCount() > 0)
    {
        on_convertButton_clicked();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    event->accept();
}

const QString SETTINGS_TAB_CURRENT = "tabWidget_current";
const QString SETTINGS_PERCENTAGE_H = "tabPercentage_hscale";
const QString SETTINGS_PERCENTAGE_V = "tabPercentage_vscale";
const QString SETTINGS_ABSOLUTE_W = "tabAbsolute_width";
const QString SETTINGS_ABSOLUTE_H = "tabAbsolute_height";
const QString SETTINGS_CONVERT_WHEN_DROP = "autoConvert_checkBox";
const QString SETTINGS_WINDOW_W = "window_width";
const QString SETTINGS_WINDOW_H = "window_height";

void MainWindow::loadSettings()
{
    QSettings settings("sindney", "batchimageresizer");

    int curIndex = settings.value(SETTINGS_TAB_CURRENT, 0).toInt();
    m_pUi->tabWidget->setCurrentIndex(curIndex);

    m_pUi->percentageTab_hScale_spinBox->setValue(settings.value(SETTINGS_PERCENTAGE_H, 50).toInt());
    m_pUi->percentageTab_vScale_spinBox->setValue(settings.value(SETTINGS_PERCENTAGE_V, 50).toInt());
    m_pUi->absoluteTab_width_spinBox->setValue(settings.value(SETTINGS_ABSOLUTE_W, 1280).toInt());
    m_pUi->absoluteTab_height_spinBox->setValue(settings.value(SETTINGS_ABSOLUTE_H, 720).toInt());
    m_pUi->autoConvert_checkBox->setChecked(settings.value(SETTINGS_CONVERT_WHEN_DROP, false).toBool());

    int winWidth = settings.value(SETTINGS_WINDOW_W).toInt();
    int winHeight = settings.value(SETTINGS_WINDOW_H).toInt();
    if (winWidth > 0 && winHeight > 0)
    {
        this->resize(winWidth, winHeight);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("sindney", "batchimageresizer");
    settings.setValue(SETTINGS_TAB_CURRENT, m_pUi->tabWidget->currentIndex());
    settings.setValue(SETTINGS_PERCENTAGE_H, m_pUi->percentageTab_hScale_spinBox->value());
    settings.setValue(SETTINGS_PERCENTAGE_V, m_pUi->percentageTab_vScale_spinBox->value());
    settings.setValue(SETTINGS_ABSOLUTE_W, m_pUi->absoluteTab_width_spinBox->value());
    settings.setValue(SETTINGS_ABSOLUTE_H, m_pUi->absoluteTab_height_spinBox->value());
    settings.setValue(SETTINGS_CONVERT_WHEN_DROP, m_pUi->autoConvert_checkBox->isChecked());
    settings.setValue(SETTINGS_WINDOW_W, this->width());
    settings.setValue(SETTINGS_WINDOW_H, this->height());
}

int MainWindow::convertImages(ConvertMode mode, int v0, int v1, const QVector<QString> &imagePaths)
{
    int count = 0;
    foreach (const QString &path, imagePaths)
    {
        QImage image;
        if (image.load(path))
        {
            int scaledWidth = 0, scaledHeight = 0;
            if (mode == ConvertMode::PERCENTAGE)
            {
                scaledWidth = (int)(image.width() * ((float)v0 / 100));
                scaledHeight = (int)(image.height() * ((float)v1 / 100));
            }
            else if (mode == ConvertMode::ABSOLUTE)
            {
                scaledWidth = (int)v0;
                scaledHeight = (int)v1;
            }

            QImage scaledImg = image.scaled(scaledWidth, scaledHeight, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

            QImageWriter writer(path);
            if (writer.canWrite() && writer.write(scaledImg))
            {
                count++;
            }
            else
            {
                qDebug() << writer.errorString();
            }
        }
    }
    return count;
}

void MainWindow::fileListModel_dataChanged()
{
    m_pUi->actionRemove_All->setEnabled(m_pFileListModel->rowCount() > 0);
}

void MainWindow::listView_selectionModel_update()
{
    m_pUi->actionRemove->setEnabled(m_pUi->listView->selectionModel()->selectedIndexes().size() > 0);
}

void MainWindow::futures_finished()
{
    int totalCount = m_pFileListModel->rowCount();
    int convertedCount = 0;
    for (int i = 0; i < m_pFutureWatcherList->futureCount(); i++)
    {
        convertedCount += m_pFutureWatcherList->getFutureAt(i).result();
    }

    m_pFutureWatcherList->reset();
    m_pFileListModel->removeRows(0, m_pFileListModel->rowCount());

    this->setEnabled(true);

    QMessageBox::information(this, "Alert", QString("%1 of %2 image files were successfully converted!").arg(convertedCount).arg(totalCount), QMessageBox::StandardButton::Ok);
}

void MainWindow::on_actionRemove_triggered()
{
    QModelIndexList indexes;
    while((indexes = m_pUi->listView->selectionModel()->selectedIndexes()).size())
    {
        m_pFileListModel->removeRow(indexes.first().row());
    }
}

void MainWindow::on_actionRemove_All_triggered()
{
    m_pFileListModel->removeRows(0, m_pFileListModel->rowCount());
}

void MainWindow::on_actionSelect_All_triggered()
{
    m_pUi->listView->selectAll();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

const QString STR_OPENFILE_FILTER = "Image Files (*.png *.jpg *.jpeg)";

const QString STR_OPENFILE_TITLE = "Open Image Files";

void MainWindow::on_actionOpen_triggered()
{
    QList<QUrl> fileUrls = QFileDialog::getOpenFileUrls(this, STR_OPENFILE_TITLE, QUrl(), STR_OPENFILE_FILTER);
    foreach (const QUrl &url, fileUrls)
    {
        QString fileName = url.toLocalFile();
        int rowCount = m_pFileListModel->rowCount();
        m_pFileListModel->insertRow(rowCount);
        m_pFileListModel->setData(m_pFileListModel->index(rowCount), fileName);
    }
}

const QString STR_ABOUT = "Batch Image Resizer\nVersion: 1.0.1\nCopyright 2018 sindney. All rights reserved.";

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About", STR_ABOUT);
}

const int TWO_THREAD_COUNT = 20;
const int FOUR_THREAD_COUNT = 40;

void MainWindow::on_convertButton_clicked()
{
    QVector<QString> imagePaths;
    for (int i = 0; i < m_pFileListModel->rowCount(); i++)
    {
        imagePaths.push_back(m_pFileListModel->data(m_pFileListModel->index(i)).toString());
    }

    int imageFileCount = imagePaths.size();
    if (imageFileCount > 0)
    {
        ConvertMode mode = ConvertMode::ABSOLUTE;
        int v0 = 0;
        int v1 = 0;

        if (m_pUi->tabWidget->currentIndex() == 0)
        {
            mode = ConvertMode::PERCENTAGE;
            v0 = m_pUi->percentageTab_hScale_spinBox->value();
            v1 = m_pUi->percentageTab_vScale_spinBox->value();
        }
        else
        {
            mode = ConvertMode::ABSOLUTE;
            v0 = m_pUi->absoluteTab_width_spinBox->value();
            v1 = m_pUi->absoluteTab_height_spinBox->value();
        }

        if (imageFileCount > FOUR_THREAD_COUNT)
        {
            int part = imageFileCount / 4;
            int part4 = imageFileCount - part * 3;
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(0, part)));
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(part, part)));
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(part * 2, part)));
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(part * 3, part4)));
        }
        else if (imageFileCount > TWO_THREAD_COUNT)
        {
            int part1 = imageFileCount / 2;
            int part2 = imageFileCount - part1;
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(0, part1)));
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths.mid(part1, part2)));
        }
        else
        {
            m_pFutureWatcherList->addFuture(QtConcurrent::run(this, &MainWindow::convertImages, mode, v0, v1, imagePaths));
        }

        this->setEnabled(false);
    }
}
