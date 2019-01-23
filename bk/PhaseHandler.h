#ifndef PHASE_HANDLER_H
#define PHASE_HANDERL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Request.h"

/*
 * this is an ABC class
 */
class PhaseHandler: boost::noncopyable
{
public:
    typedef boost::function<int(/*TODO*/)> PhaseHandlerPt;
    typedef boost::function<int(/*TODO*/)> HandlerPt;

    PhaseHandler(PhaseHanderPt):
        r_(r) {}

    virtual ~PhaseHandler() {};
    virtual int Handle() = 0;

private:
    boost::shared
    PhaseHandlerPt checker_;
    HandlerPt      handler_;
    uint32_t       phase_;
    uint32_t       next_;
}
