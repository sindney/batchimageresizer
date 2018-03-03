#include "qfuturewatcherlist.h"

QFutureWatcherList::QFutureWatcherList()
    : m_FinishedFutureCount(0)
{

}

void QFutureWatcherList::addFuture(const QFuture<int> &future)
{
    m_FutureList.push_back(future);
    QFutureWatcher<int> *watcher = new QFutureWatcher<int>();
    connect(watcher, SIGNAL(finished()), this, SLOT(future_finished()));
    watcher->setFuture(future);
    m_FutureWatcherList.push_back(watcher);
}

int QFutureWatcherList::futureCount() const
{
    return m_FutureList.count();
}

QFuture<int> QFutureWatcherList::getFutureAt(int index) const
{
    return m_FutureList.at(index);
}

void QFutureWatcherList::reset()
{
    m_FinishedFutureCount = 0;
    m_FutureList.clear();
    foreach (const QFutureWatcher<int> *ptr, m_FutureWatcherList)
    {
        delete ptr;
    }
    m_FutureWatcherList.clear();
}

void QFutureWatcherList::future_finished()
{
    m_FinishedFutureCount++;
    if (m_FinishedFutureCount == m_FutureList.count())
    {
        emit futures_finished();
    }
}
