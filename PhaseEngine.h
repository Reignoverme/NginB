#ifndef PHASE_ENGINE_H
#define PHASE_ENGINE_H
#include <list>
#include <ctime>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

class PhaseHandler;

class PhaseEngine : boost::noncopyable
{
public:
    typedef boost::shared_ptr<PhaseHandler> Handler;

    PhaseEngine(const std::list<PhaseHandler> handlers):
      handlers_(handlers) 
    {}

private:
    typedef std::list<Handler> HandlerList;

    HandlerList handlers_;
};

extern PhaseEngine ph;

#endif
