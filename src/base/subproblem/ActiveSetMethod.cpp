#include <cmath>
#include <map>
#include "ActiveSetMethod.hpp"
#include "Constraint.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

ActiveSetMethod::ActiveSetMethod(QPSolver& solver) : Subproblem("l1"), solver(solver) {
}

Iterate ActiveSetMethod::initialize(Problem& problem, std::vector<double>& x, Multipliers& multipliers, int number_variables, bool /*use_trust_region*/) {
    // register the original bounds
    this->subproblem_variables_bounds = problem.variables_bounds;

    Iterate first_iterate(x, multipliers);
    /* compute the optimality and feasibility measures of the initial point */
    this->compute_optimality_measures(problem, first_iterate);

    /* allocate the QP solver */
    this->solver.allocate(number_variables, problem.number_constraints);
    return first_iterate;
}

SubproblemSolution ActiveSetMethod::compute_optimality_step(Problem& problem, Iterate& current_iterate, double trust_region_radius) {
    /* evaluate the functions at the current iterate */
    this->evaluate_optimality_iterate(problem, current_iterate);

    /* bounds of the variables */
    std::vector<Range> variables_bounds = this->generate_variables_bounds(current_iterate, trust_region_radius);

    /* bounds of the linearized constraints */
    std::vector<Range> constraints_bounds = Subproblem::generate_constraints_bounds(problem, current_iterate.constraints);

    /* generate the initial point */
    std::vector<double> d0(current_iterate.x.size()); // = {0.}

    /* solve the QP */
    SubproblemSolution solution = this->solve_subproblem(variables_bounds, constraints_bounds, current_iterate, d0);
    solution.phase_1_required = this->phase_1_required(solution);
    this->number_subproblems_solved++;
    return solution;
}

SubproblemSolution ActiveSetMethod::compute_infeasibility_step(Problem& problem, Iterate& current_iterate, SubproblemSolution& phase_II_solution, double trust_region_radius) {
    DEBUG << "Creating the restoration problem with " << phase_II_solution.constraint_partition.infeasible.size() << " infeasible constraints\n";
    
    /* evaluate the functions at the current iterate */
    this->evaluate_feasibility_iterate(problem, current_iterate, phase_II_solution);
    /* compute the objective */
    this->compute_linear_feasibility_objective(current_iterate, phase_II_solution.constraint_partition);

    /* bounds of the variables */
    std::vector<Range> variables_bounds = this->generate_variables_bounds(current_iterate, trust_region_radius);

    /* bounds of the linearized constraints */
    std::vector<Range> constraints_bounds = this->generate_feasibility_bounds(problem, current_iterate.constraints, phase_II_solution.constraint_partition);

    /* generate the initial point */
    std::vector<double> d0 = phase_II_solution.x;

    /* solve the QP */
    SubproblemSolution solution = this->solve_subproblem(variables_bounds, constraints_bounds, current_iterate, d0);
    solution.constraint_partition = phase_II_solution.constraint_partition;
    this->number_subproblems_solved++;
    return solution;
}

void ActiveSetMethod::compute_optimality_measures(Problem& problem, Iterate& iterate) {
    // feasibility
    iterate.compute_constraints_residual(problem, this->residual_norm);
    iterate.feasibility_measure = iterate.residual;
    // optimality
    iterate.compute_objective(problem);
    iterate.optimality_measure = iterate.objective;
    return;
}

void ActiveSetMethod::compute_infeasibility_measures(Problem& problem, Iterate& iterate, SubproblemSolution& solution) {
    iterate.compute_constraints(problem);
    iterate.feasibility_measure = problem.feasible_residual_norm(solution.constraint_partition, iterate.constraints, this->residual_norm);
    iterate.optimality_measure = problem.infeasible_residual_norm(solution.constraint_partition, iterate.constraints, this->residual_norm);
    return;
}

/* private methods */

std::vector<Range> ActiveSetMethod::generate_feasibility_bounds(Problem& problem, std::vector<double>& current_constraints, ConstraintPartition& constraint_partition) {
    std::vector<Range> constraints_bounds(problem.number_constraints);
    for (int j = 0; j < problem.number_constraints; j++) {
        double lb, ub;
        if (constraint_partition.constraint_feasibility[j] == INFEASIBLE_LOWER) {
            lb = -INFINITY;
            ub = problem.constraints_bounds[j].lb - current_constraints[j];
        }
        else if (constraint_partition.constraint_feasibility[j] == INFEASIBLE_UPPER) {
            lb = problem.constraints_bounds[j].ub - current_constraints[j];
            ub = INFINITY;
        }
        else { // FEASIBLE
            lb = problem.constraints_bounds[j].lb - current_constraints[j];
            ub = problem.constraints_bounds[j].ub - current_constraints[j];
        }
        constraints_bounds[j] = {lb, ub};
    }
    return constraints_bounds;
}

void ActiveSetMethod::compute_linear_feasibility_objective(Iterate& current_iterate, ConstraintPartition& constraint_partition) {
    /* objective function: sum of gradients of infeasible constraints */
    std::map<int, double> objective_gradient;
    for (int j : constraint_partition.infeasible) {
        for (std::pair<int, double> term : current_iterate.constraints_jacobian[j]) {
            int i = term.first;
            double derivative = term.second;

            if (constraint_partition.constraint_feasibility[j] == INFEASIBLE_LOWER) {
                objective_gradient[i] -= derivative;
            }
            else {
                objective_gradient[i] += derivative;
            }
        }
    }
    current_iterate.set_objective_gradient(objective_gradient);
    return;
}