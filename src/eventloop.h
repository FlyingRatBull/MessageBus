#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <QEventLoop>

class EventLoop : public QEventLoop
{
    Q_OBJECT

  public:
    EventLoop();
    
    virtual ~EventLoop();
    
  public slots:
    void abort();
};

#endif // EVENTLOOP_H
