#ifndef QFUTUREWATCHERLIST_H
#define QFUTUREWATCHERLIST_H

#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>

class QFutureWatcherList : public QObject
{
    Q_OBJECT

public:
    QFutureWatcherList();
    void addFuture(const QFuture<int> &future);
    int futureCount() const;
    QFuture<int> getFutureAt(int index) const;
    void reset();

signals:
    void futures_finished();

private slots:
    void future_finished();

private:
    QVector<QFuture<int>> m_FutureList;
    QVector<QFutureWatcher<int>*> m_FutureWatcherList;
    int m_FinishedFutureCount;
};

#endif // QFUTUREWATCHERLIST_H
