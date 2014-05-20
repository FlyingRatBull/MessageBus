#include "eventloop.h"

EventLoop::EventLoop(): QEventLoop()
{

}


EventLoop::~EventLoop()
{

}


void EventLoop::abort()
{
  QEventLoop::exit(1);
}
