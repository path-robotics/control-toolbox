/***********************************************************************************
Copyright (c) 2017, Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo,
Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
 * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************************/

#pragma once

namespace ct {
namespace rbd {

template <class RBDDynamics, typename SCALAR>
FixBaseNLOC<RBDDynamics, SCALAR>::FixBaseNLOC(const std::string& costFunctionFile,
    const std::string& settingsFile,
    std::shared_ptr<FBSystem> system,
    bool verbose,
    std::shared_ptr<LinearizedSystem> linearizedSystem)
    : system_(system),
      linearizedSystem_(linearizedSystem),
      costFunction_(new CostFunction(costFunctionFile, verbose)),
      optConProblem_(system_, costFunction_, linearizedSystem_),
      iteration_(0)
{
    optConProblem_.verify();
    nlocSolver_ = std::shared_ptr<NLOptConSolver>(new NLOptConSolver(optConProblem_, settingsFile));
}

template <class RBDDynamics, typename SCALAR>
FixBaseNLOC<RBDDynamics, SCALAR>::FixBaseNLOC(const std::string& costFunctionFile,
    const typename NLOptConSolver::Settings_t& nlocSettings,
    std::shared_ptr<FBSystem> system,
    bool verbose,
    std::shared_ptr<LinearizedSystem> linearizedSystem)
    : system_(system),
      linearizedSystem_(linearizedSystem),
      costFunction_(new CostFunction(costFunctionFile, verbose)),
      optConProblem_(system_, costFunction_, linearizedSystem_),
      iteration_(0)
{
    optConProblem_.verify();
    nlocSolver_ = std::shared_ptr<NLOptConSolver>(new NLOptConSolver(optConProblem_, nlocSettings));
}

template <class RBDDynamics, typename SCALAR>
void FixBaseNLOC<RBDDynamics, SCALAR>::initialize(const tpl::JointState<FBSystem::CONTROL_DIM, SCALAR>& x0,
    const core::Time& tf,
    StateVectorArray x_ref,
    FeedbackArray u0_fb,
    ControlVectorArray u0_ff)
{
    typename NLOptConSolver::Policy_t policy(x_ref, u0_ff, u0_fb, getSettings().dt);

    nlocSolver_->changeTimeHorizon(tf);
    nlocSolver_->setInitialGuess(policy);
    nlocSolver_->changeInitialState(x0.toImplementation());
}

template <class RBDDynamics, typename SCALAR>
void FixBaseNLOC<RBDDynamics, SCALAR>::initializeSteadyPose(const tpl::JointState<FBSystem::CONTROL_DIM, SCALAR>& x0,
    const core::Time& tf,
    const int N,
    FeedbackMatrix K)
{
    StateVectorArray x_ref = StateVectorArray(N + 1, x0.toImplementation());

    ControlVector uff;
    computeIDTorques(x0, uff);
    ControlVectorArray u0_ff = ControlVectorArray(N, uff);

    FeedbackArray u0_fb(N, K);

    typename NLOptConSolver::Policy_t policy(x_ref, u0_ff, u0_fb, getSettings().dt);

    nlocSolver_->changeTimeHorizon(tf);
    nlocSolver_->setInitialGuess(policy);
    nlocSolver_->changeInitialState(x0.toImplementation());
}

template <class RBDDynamics, typename SCALAR>
void FixBaseNLOC<RBDDynamics, SCALAR>::initializeDirectInterpolation(
    const tpl::JointState<FBSystem::CONTROL_DIM, SCALAR>& x0,
    const tpl::JointState<FBSystem::CONTROL_DIM, SCALAR>& xf,
    const core::Time& tf,
    const int N,
    FeedbackMatrix K)
{
    ControlVector uff;

    StateVectorArray x_ref = StateVectorArray(N + 1);
    ControlVectorArray u0_ff = ControlVectorArray(N);

    for (int i = 0; i < N + 1; i++)
    {
        x_ref[i] = x0.toImplementation() + (xf.toImplementation() - x0.toImplementation()) * SCALAR(i) / SCALAR(N);

        if (i < N)
            computeIDTorques(x_ref[i], u0_ff[i]);
    }

    FeedbackArray u0_fb(N, K);

    typename NLOptConSolver::Policy_t policy(x_ref, u0_ff, u0_fb, getSettings().dt);

    nlocSolver_->changeTimeHorizon(tf);
    nlocSolver_->setInitialGuess(policy);
    nlocSolver_->changeInitialState(x0.toImplementation());
}

template <class RBDDynamics, typename SCALAR>
bool FixBaseNLOC<RBDDynamics, SCALAR>::runIteration()
{
    bool foundBetter = nlocSolver_->runIteration();

    iteration_++;
    return foundBetter;
}

template <class RBDDynamics, typename SCALAR>
const core::StateFeedbackController<FixBaseNLOC<RBDDynamics, SCALAR>::FBSystem::STATE_DIM,
    FixBaseNLOC<RBDDynamics, SCALAR>::FBSystem::CONTROL_DIM,
    SCALAR>&
FixBaseNLOC<RBDDynamics, SCALAR>::getSolution()
{
    return nlocSolver_->getSolution();
}

template <class RBDDynamics, typename SCALAR>
const typename FixBaseNLOC<RBDDynamics, SCALAR>::StateVectorArray&
FixBaseNLOC<RBDDynamics, SCALAR>::retrieveLastRollout()
{
    return nlocSolver_->getStates();
}

template <class RBDDynamics, typename SCALAR>
const core::TimeArray& FixBaseNLOC<RBDDynamics, SCALAR>::getTimeArray()
{
    return nlocSolver_->getStateTrajectory().getTimeArray();
}

template <class RBDDynamics, typename SCALAR>
const typename FixBaseNLOC<RBDDynamics, SCALAR>::FeedbackArray& FixBaseNLOC<RBDDynamics, SCALAR>::getFeedbackArray()
{
    return nlocSolver_->getSolution().K();
}

template <class RBDDynamics, typename SCALAR>
const typename FixBaseNLOC<RBDDynamics, SCALAR>::ControlVectorArray&
FixBaseNLOC<RBDDynamics, SCALAR>::getControlVectorArray()
{
    return nlocSolver_->getSolution().uff();
}

template <class RBDDynamics, typename SCALAR>
const typename FixBaseNLOC<RBDDynamics, SCALAR>::NLOptConSolver::Settings_t&
FixBaseNLOC<RBDDynamics, SCALAR>::getSettings() const
{
    return nlocSolver_->getSettings();
}

template <class RBDDynamics, typename SCALAR>
void FixBaseNLOC<RBDDynamics, SCALAR>::changeCostFunction(std::shared_ptr<CostFunction> costFunction)
{
    nlocSolver_->changeCostFunction(costFunction);
}

template <class RBDDynamics, typename SCALAR>
std::shared_ptr<typename FixBaseNLOC<RBDDynamics, SCALAR>::NLOptConSolver> FixBaseNLOC<RBDDynamics, SCALAR>::getSolver()
{
    return nlocSolver_;
}

template <class RBDDynamics, typename SCALAR>
void FixBaseNLOC<RBDDynamics, SCALAR>::computeIDTorques(const tpl::JointState<FBSystem::CONTROL_DIM, SCALAR>& x,
    ControlVector& u)
{
    //! zero external link forces and acceleration
    typename RBDDynamics::ExtLinkForces_t linkForces(Eigen::Matrix<SCALAR, 6, 1>::Zero());
    typename RBDDynamics::JointAcceleration_t jAcc(Eigen::Matrix<SCALAR, 6, 1>::Zero());

    system_->dynamics().FixBaseID(x, jAcc, linkForces, u);
}

}  // namespace rbd
}  // namespace ct