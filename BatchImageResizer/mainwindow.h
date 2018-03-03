#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "qfuturewatcherlist.h"

namespace Ui {
class MainWindow;
}

class QAbstractListModel;

class QString;

class QStringList;

enum class ConvertMode
{
    PERCENTAGE,
    ABSOLUTE,
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:

    void loadSettings();
    void saveSettings();

    int convertImages(ConvertMode mode, int v0, int v1, const QVector<QString> &imagePaths);

private slots:
    void fileListModel_dataChanged();
    void listView_selectionModel_update();
    void futures_finished();

    void on_actionRemove_triggered();
    void on_actionRemove_All_triggered();
    void on_actionSelect_All_triggered();
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionAbout_triggered();

    void on_convertButton_clicked();

private:
    Ui::MainWindow *m_pUi;

    QAbstractListModel *m_pFileListModel;

    QFutureWatcherList *m_pFutureWatcherList;
};

#endif // MAINWINDOW_H
