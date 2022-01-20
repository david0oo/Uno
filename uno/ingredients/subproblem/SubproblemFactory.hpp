#ifndef UNO_SUBPROBLEMFACTORY_H
#define UNO_SUBPROBLEMFACTORY_H

#include <memory>
#include "Subproblem.hpp"
#include "tools/Options.hpp"

class SubproblemFactory {
	public:
		static std::unique_ptr<Subproblem> create(const Problem& problem, const Scaling& scaling, size_t max_number_variables, const Options& options);
};

#endif // UNO_SUBPROBLEMFACTORY_H
