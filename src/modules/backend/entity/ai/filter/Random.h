/**
 * @file
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"

namespace backend {

/**
 * @brief This filter will preserve only a few random entries
 */
class Random: public IFilter {
protected:
	Filters _filters;
	int _n;
public:
	Random(const core::String& parameters, const Filters& filters);

	FILTER_ACTION_FACTORY(Random)

	void filter (const AIPtr& entity) override;
};

}
