#pragma once

#include <functional>
#include <memory>

#ifdef USE_COROUTINE_V2
#include <boost/coroutine2/coroutine.hpp>
#else
#include <boost/coroutine/coroutine.hpp>
#endif


class ofxCoroutine {
public:
	void step() {
		_coroutines_next.clear();
		for (int i = 0; i < _coroutines.size(); ++i) {
			_coroutines[i]();
			if (_coroutines[i]) {
				_coroutines_next.emplace_back(std::move(_coroutines[i]));
			}
		}
		std::swap(_coroutines_next, _coroutines);
	}

#ifdef USE_COROUTINE_V2
	typedef boost::coroutines2::asymmetric_coroutine<void>::pull_type Coroutine;
	typedef boost::coroutines2::asymmetric_coroutine<void>::push_type Yield;
#else
	typedef boost::coroutines::asymmetric_coroutine<void>::pull_type Coroutine;
	typedef boost::coroutines::asymmetric_coroutine<void>::push_type Yield;
#endif

	void add(std::function<void(Yield &)> routine) {
		Coroutine coroutine(routine);
		if (routine) {
			_coroutines.emplace_back(std::move(coroutine));
		}
	}
	void add(std::function<void(Yield &)> routine, std::size_t stack_size) {
#ifdef USE_COROUTINE_V2
		Coroutine coroutine(boost::coroutines2::fixedsize_stack(stack_size), routine);
#else
		Coroutine coroutine(routine, boost::coroutines::attributes(stack_size));
#endif
		if (routine) {
			_coroutines.emplace_back(std::move(coroutine));
		}
	}

	std::vector<Coroutine> _coroutines;
	std::vector<Coroutine> _coroutines_next;
};