#ifndef PHASE_ENGINE_H
#define PHASE_ENGINE_H
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

class PhaseHandler;

class PhaseEngine : boost::noncopyable
{
public:
    void

privat:
    typedef std::vector<const PhaseHandler&> HandlerList;

    HandlerList handlers;
}
