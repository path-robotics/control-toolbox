/**********************************************************************************************************************
This file is part of the Control Toolbox (https://github.com/ethz-adrl/control-toolbox), copyright by ETH Zurich.
Licensed under the BSD-2 license (see LICENSE file in main directory)
**********************************************************************************************************************/

#pragma once

#include <ct/optcon/solver/NLOptConSettings.hpp>

#include <ct/optcon/problem/LQOCProblem.hpp>

namespace ct {
namespace optcon {

/*!
 * Base class for solvers to solve an LQOCProblem
 * (both constrained / unconstrained, etc.)
 *
 * \todo uncouple from NLOptConSettings
 */
template <size_t STATE_DIM, size_t CONTROL_DIM, typename SCALAR = double>
class LQOCSolver
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    typedef LQOCProblem<STATE_DIM, CONTROL_DIM, SCALAR> LQOCProblem_t;

    /*!
	 * Constructor. Initialize by handing over an LQOCProblem, or otherwise by calling setProblem()
	 * @param lqocProblem shared_ptr to the LQOCProblem to be solved.
	 */
    LQOCSolver(const std::shared_ptr<LQOCProblem_t>& lqocProblem = nullptr) : lqocProblem_(lqocProblem) {}

    //! destructor
    virtual ~LQOCSolver() {}
    /*!
	 * set a new problem
	 * update the shared_ptr to the LQOCProblem instance and call initialize instance deriving from this class.
	 * @param lqocProblem
	 */
    
    void setProblem(std::shared_ptr<LQOCProblem_t> lqocProblem)
    {
        setProblemImpl(lqocProblem);
        lqocProblem_ = lqocProblem;
    }

    virtual void configure(const NLOptConSettings& settings) = 0;

    //! setup and configure the box constraints
    virtual void configureInputBoxConstraints(std::shared_ptr<LQOCProblem<STATE_DIM, CONTROL_DIM>> lqocProblem)
    {
        throw std::runtime_error("input box constraints are not available for this solver.");
    }

    virtual void configureStateBoxConstraints(std::shared_ptr<LQOCProblem<STATE_DIM, CONTROL_DIM>> lqocProblem)
    {
        throw std::runtime_error("state box constraints are not available for this solver.");
    }

    //! setup and configure the general (in)equality constraints
    virtual void configureGeneralConstraints(std::shared_ptr<LQOCProblem<STATE_DIM, CONTROL_DIM>> lqocProblem)
    {
        throw std::runtime_error("general constraints are not available for this solver.");
    }

    //! a method reserved for memory allocation (e.g. required for HPIPM)
    virtual void initializeAndAllocate() = 0;

    //! solve the LQOC problem
    virtual void solve() = 0;

    //! extract the solution (can be overriden if additional extraction steps required in specific solver)
    virtual void extractLQSolution(){};

    virtual void solveSingleStage(int N)
    {
        throw std::runtime_error("solveSingleStage not available for this solver.");
    }

    virtual SCALAR getSmallestEigenvalue()
    {
        throw std::runtime_error("getSmallestEigenvalue not available for this solver.");
    }

    const ct::core::StateVectorArray<STATE_DIM, SCALAR>& getSolutionState() { return x_sol_; }

    const ct::core::ControlVectorArray<CONTROL_DIM, SCALAR>& getSolutionControl() { return u_sol_; }
    
    const ct::core::FeedbackArray<STATE_DIM, CONTROL_DIM, SCALAR>& getSolutionFeedback() { return L_; }

    virtual void get_lv(ct::core::ControlVectorArray<CONTROL_DIM, SCALAR>& lv) { 
        throw std::runtime_error("Lv not supported by this LQ solver....");
     }

protected:
    virtual void setProblemImpl(std::shared_ptr<LQOCProblem_t> lqocProblem) = 0;

    std::shared_ptr<LQOCProblem_t> lqocProblem_;

    core::StateVectorArray<STATE_DIM, SCALAR> x_sol_;            // solution in x
    core::ControlVectorArray<CONTROL_DIM, SCALAR> u_sol_;        // solution in u
    ct::core::FeedbackArray<STATE_DIM, CONTROL_DIM, SCALAR> L_;  // solution feedback
};
}  // namespace optcon
}  // namespace ct
