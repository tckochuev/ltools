#pragma once

#include <atomic>

#include "VSUtils.h"
#include "VSError.h"

///@brief Interface for class with interruptible operation.
/// Interruptible operation should check interrupt state from time to time(period is implementation defined)
/// and throw Interrupted exception in case of positive check.
class VSIInterruptible
{
	INTERFACE(VSIInterruptible)
public:
	using Interrupted = tc::err::exc::Interrupted;

	virtual bool getInterruptionFlag() const = 0;
	virtual void setInterruptionFlag(bool) = 0;

	void checkInterrupt() const {
		if(getInterruptionFlag()) {
			throw Interrupted();
		}
	}
};

class VSStdAtomicBoolInterruptor : public VSIInterruptible
{
public:
	VSStdAtomicBoolInterruptor(bool interrupt = false) : m_interrupt(interrupt)
	{}

	bool getInterruptionFlag() const override {
		return m_interrupt;
	}
	void setInterruptionFlag(bool interrupt) override {
		m_interrupt = interrupt;
	}

private:
	std::atomic_bool m_interrupt = false;
};
