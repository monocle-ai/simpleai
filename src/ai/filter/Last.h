#pragma once

#include "filter/IFilter.h"

namespace ai {

/**
 * @brief This filter will just preserve the last entry of other filters
 */
class Last: public IFilter {
public:
	FILTER_ACTION_CLASS(Last)
	FILTER_ACTION_FACTORY(Last)

	void filter (const AIPtr& entity) override;
};

inline void Last::filter (const AIPtr& entity) {
	FilteredEntities& filtered = getFilteredEntities(entity);
	const FilteredEntities copy = filtered;
	filtered.clear();
	_filters.front()->filter(entity);
	const auto& value = getFilteredEntities(entity).back();
	filtered.clear();
	for (auto& e : copy) {
		filtered.push_back(e);
	}
	filtered.push_back(value);
}

}
