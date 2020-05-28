#include <cmath>
#include "TrustRegion.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "AMPLModel.hpp"

TrustRegion::TrustRegion(GlobalizationStrategy& globalization_strategy, double tolerance, double initial_radius, int max_iterations):
GlobalizationMechanism(globalization_strategy, tolerance, max_iterations), radius(initial_radius), activity_tolerance_(1e-6) {
}

Iterate TrustRegion::initialize(Problem& problem, std::vector<double>& x, Multipliers& multipliers) {
    return this->globalization_strategy.initialize(problem, x, multipliers);
}

Iterate TrustRegion::compute_acceptable_iterate(Problem& problem, Iterate& current_iterate) {
    bool is_accepted = false;
    this->number_iterations = 0;

    while (!this->termination_(is_accepted)) {
        try {
            this->number_iterations++;
            this->print_iteration_();

            /* compute the step within trust region */
            SubproblemSolution solution = this->globalization_strategy.subproblem.compute_step(problem, current_iterate, this->radius);
            
            /* set bound multipliers of active trust region to 0 */
            this->correct_multipliers_(solution, this->radius);
            
            /* check whether the trial step is accepted */
            is_accepted = this->globalization_strategy.check_step(problem, current_iterate, solution);
            
            if (is_accepted) {
                current_iterate.status = this->compute_status_(problem, current_iterate, solution.norm, solution.objective_multiplier);
                /* print summary */
                this->print_acceptance_(solution.norm);

                /* increase the radius if trust region is active, otherwise keep the same radius */
                if (solution.norm >= this->radius - this->activity_tolerance_) {
                    this->radius *= 2.;
                }
            }
            else {
                /* if the step is rejected, decrease the radius */
                this->radius = std::min(this->radius, solution.norm) / 2.;
            }
        }
        catch (const IEEE_Error& e) {
            this->print_warning_(e.what());
            /* if an evaluation error occurs, decrease the radius */
            this->radius /= 2.;
        }
    }
    return current_iterate;
}

void TrustRegion::correct_multipliers_(SubproblemSolution& solution, double radius) {
    /* set multipliers for bound constraints active at trust region to 0 */
    for (int i: solution.active_set.bounds.at_upper_bound) {
        if (solution.x[i] == radius) {
            solution.multipliers.upper_bounds[i] = 0.;
        }
    }
    for (int i: solution.active_set.bounds.at_lower_bound) {
        if (solution.x[i] == -radius) {
            solution.multipliers.lower_bounds[i] = 0.;
        }
    }
    return;
}

bool TrustRegion::termination_(bool is_accepted) {
    if (is_accepted) {
        return true;
    }
    else if (this->max_iterations < this->number_iterations) {
        throw std::runtime_error("Trust-region iteration limit reached");
    } /* radius gets too small */
    else if (this->radius < 1e-16) { /* 1e-16: something like machine precision */
        throw std::runtime_error("Trust-region radius became too small");
    }
    return false;
}

void TrustRegion::print_iteration_() {
    DEBUG << "\n\tTRUST REGION iteration " << this->number_iterations << ", radius " << this->radius << "\n";
    return;
}

void TrustRegion::print_acceptance_(double solution_norm) {
    DEBUG << CYAN "TR trial point accepted\n" RESET;
    INFO << "minor: " << this->number_iterations << "\t";
    INFO << "radius: " << this->radius << "\t\t";
    INFO << "step norm: " << solution_norm << "\t\t";
    return;
}

void TrustRegion::print_warning_(const char* message) {
    WARNING << RED << message << RESET << "\n";
    return;
}
