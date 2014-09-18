/* -------------------------------------------------------------------------- *
 *               Simbody(tm) Example: UR10 Task Space Control                 *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK biosimulation toolkit originating from           *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org/home/simbody.  *
 *                                                                            *
 * Portions copyright (c) 2014 Stanford University and the Authors.           *
 * Authors: Chris Dembia, Michael Sherman                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

/* This example shows a task space (a.k.a. operational space) controller for
a robot arm in Simbody. Task space controllers are model-based, meaning that
the controller itself contains a model of the system being controlled. Since
we don't have a real robot handy, that means there will be two versions of
the UR10 system here: one that we're simulating that is taking the role of the
"real" robot, and one contained in the task space controller. In real life the
internal model won't perfectly match the real one; we'll fake that here by
changing some parameters between the two models.
We assume the system has a motor at each of its degrees of freedom.

The task the controller will achieve has two components:
1. The arm reaches for a target point that can be moved with arrow keys.
2. All links are subject to gravity compensation (to counteract the effect
of gravity).

For more information about operational space control, see:

Khatib, Oussama, et al. "Robotics-based synthesis of human motion."
Journal of physiology-Paris 103.3 (2009): 211-219.
*/

#include "Simbody.h"
#include "UR10.h"

#include <iostream>

using namespace SimTK;
using namespace std;


template <class T>
class TasksMeasure : public Measure_<T> {
public:
    SimTK_MEASURE_HANDLE_PREAMBLE(TasksMeasure, Measure_<T>);

    TasksMeasure(UR10& modelRobot) 
    :   Measure_<T>(modelRobot.updForceSubsystem(), 
                    new Implementation(modelRobot),
                    AbstractMeasure::SetHandle()) {}


    const Vec3& getTarget() const { return getImpl().m_desiredTaskPosInGround; }
    Vec3& updTarget() { return updImpl().m_desiredTaskPosInGround; }
    void setTarget(Vec3 pos) { updImpl().m_desiredTaskPosInGround = pos; }

    void toggleGravityComp() {
        updImpl().m_compensateForGravity = !isGravityCompensationOn();}
    void toggleTask() {updImpl().m_controlTask = !getImpl().m_controlTask;}

    bool isGravityCompensationOn() const 
    {   return getImpl().m_compensateForGravity; }
    const Vec3& getTaskPointInEndEffector() const 
    {   return getImpl().m_taskPointInEndEffector; }

    SimTK_MEASURE_HANDLE_POSTSCRIPT(TasksMeasure, Measure_<T>);
};


template <class T>
class TasksMeasure<T>::Implementation : public Measure_<T>::Implementation {
public:
    Implementation(const UR10& modelRobot,
                   Vec3 taskPointInEndEffector=Vec3(0,0,0),
                   Real proportionalGain=100, double derivativeGain=20) 
    :   Measure_<T>::Implementation(T(UR10::NumCoords,NaN), 1),
        m_modelRobot(modelRobot),
        m_tspace1(m_modelRobot.getMatterSubsystem(), m_modelRobot.getGravity()),
        m_tspace2(m_modelRobot.getMatterSubsystem(), m_modelRobot.getGravity()),
        m_taskPointInEndEffector(taskPointInEndEffector),
        m_taskPointInOther(Vec3(.1,.1,.5)),
        m_proportionalGain(proportionalGain),
        m_derivativeGain(derivativeGain),
        m_dampingGain(1),
        m_compensateForGravity(true),
        m_controlTask(true),
        m_desiredTaskPosInGround(Vec3(-0.4, 0.1, 1)), // Z is up
        m_desiredOtherPosInGround(Vec3(.1,.1,.5)) 
    {       
        m_tspace1.addStationTask(m_modelRobot.getBody(UR10::EndEffector),
                         m_taskPointInEndEffector);
        m_tspace2.addStationTask(m_modelRobot.getBody(UR10::Forearm),
                         m_taskPointInOther);
    }


    // Default copy constructor, destructor, copy assignment are fine.

    // Implementations of virtual methods.
    Implementation* cloneVirtual() const OVERRIDE_11
    {   return new Implementation(*this); }
    int getNumTimeDerivativesVirtual() const OVERRIDE_11 {return 0;}
    Stage getDependsOnStageVirtual(int order) const OVERRIDE_11
    {   return Stage::Velocity; }

    void calcCachedValueVirtual(const State& s, int derivOrder, T& value) const
        OVERRIDE_11;

    void realizeMeasureTopologyVirtual(State& modelState) const OVERRIDE_11 {
        m_tspace1.realizeTopology(modelState);
        m_tspace2.realizeTopology(modelState);
    }
private:
friend class TasksMeasure<T>;

    const UR10&     m_modelRobot;
    TaskSpace       m_tspace1;
    TaskSpace       m_tspace2;

    const Vec3      m_taskPointInEndEffector;
    const Vec3      m_taskPointInOther;
    const Real      m_proportionalGain;
    const Real      m_derivativeGain;
    const Real      m_dampingGain;

    bool            m_compensateForGravity;
    bool            m_controlTask;
    Vec3            m_desiredTaskPosInGround;
    Vec3            m_desiredOtherPosInGround;
};

//------------------------------------------------------------------------------
//                TASKS MEASURE :: CALC CACHED VALUE VIRTUAL
//------------------------------------------------------------------------------
// Given a modelState that has been updated from the real robot's sensors, 
// generate control torques as the TasksMeasure's value.

template <class T>
void TasksMeasure<T>::Implementation::calcCachedValueVirtual
   (const State& modelState, int derivOrder, T& tau) const
{
    SimTK_ASSERT1_ALWAYS(derivOrder==0,
        "TasksMeasure::Implementation::calcCachedValueVirtual():"
        " derivOrder %d seen but only 0 allowed.", derivOrder);

    // Shorthands.
    // -----------
    const TaskSpace& p1 = m_tspace1;
    const TaskSpace& p2 = m_tspace2;

    const int nu = tau.size();

    const Real& kd = m_derivativeGain;
    const Real& kp = m_proportionalGain;
    const Vec3& x1_des = m_desiredTaskPosInGround;
    const Vec3& x2_des = m_desiredOtherPosInGround;


    // Abbreviate model state for convenience.
    const State& ms = modelState;

    // Compute control law in task space (F*).
    // ---------------------------------------
    Vec3 xd_des(0);
    Vec3 xdd_des(0);

    // Get info about the actual location, etc. of the left hand.
    Vec3 x1, x1d;
    p1.findStationLocationAndVelocityInGround(ms,
            TaskSpace::StationTaskIndex(0),
            m_taskPointInEndEffector, x1, x1d);
    Vec3 x2, x2d;
    p2.findStationLocationAndVelocityInGround(ms,
            TaskSpace::StationTaskIndex(0),
            m_taskPointInOther, x2, x2d);

    // Units of acceleration.
    Vec3 Fstar1 = xdd_des + kd * (xd_des - x1d) + kp * (x1_des - x1);
    Vector Fstar2(xdd_des + kd * (xd_des - x2d) + kp * (x2_des - x2));

    // Compute task-space force that achieves the task-space control.
    // F = Lambda Fstar + p
    Vector F1 = p1.Lambda(ms) * Fstar1 + p1.mu(ms) + p1.p(ms);
    Vector F2 = p2.calcInverseDynamics(ms, Fstar2);

    //cout << "F1=" << F1 << endl;
    //cout << "F2=" << F2 << endl;
    //cout << "g=" << p1.g(s) << endl;

    // Combine the reaching task with the gravity compensation and nullspace
    // damping.
    // Gamma = J1T F1 + N1T J1T F2 + N1T N2T (g - k u)
    const Vector& u = ms.getU();
    const Real c = m_dampingGain/10;
    tau.setToZero();
    //tau +=   p1.JT(s) * F1 
    //              + p1.NT(s) * (  p2.JT(s) * F2 
    //                            + p2.NT(s) * (p1.g(s) - c * u));


    if (m_controlTask) {
        tau += p1.JT(ms) * F1;
        if (m_compensateForGravity)
            tau += p1.NT(ms) * p1.g(ms);
        tau -= p1.NT(ms) * (c*u); // damping
    } else if (m_compensateForGravity) {
        tau += p1.g(ms);
        tau -= p1.NT(ms) * (c*u); // damping
    } else 
        tau += -c*u;

    UR10::clampToLimits(tau);
}

//==============================================================================
//                    REACHING AND GRAVITY COMPENSATION
//==============================================================================
// This is a task-space controller that tries to move the end effector to 
// a particular target location, and applies gravity compensation and some
// joint damping as lower-priority tasks.
//
// The controller has its own internal UR10 model which in general does not
// match the "real" UR10 perfectly. Each time it is asked to
// generate control torques it reads the sensors on the real UR10, updates
// its internal model to match. It then generates torques that would be right
// for the internal model, but returns them to be applied to the real UR10.
class ReachingAndGravityCompensation : public Force::Custom::Implementation {
public:
    // The arm reaches for a target location, and the controller
    // compensates for gravity.
    // 
    // @param[in] taskPointInEndEffector The point whose location we want
    //                                   to control.
    // @param[in] proportionalGain Units of N-m/rad
    // @param[in] derivativeGain Units of N-m-s/rad
    ReachingAndGravityCompensation(const UR10& realRobot,
            Vec3 taskPointInEndEffector=Vec3(0,0,0),
            double proportionalGain=100, double derivativeGain=20) :
        m_modelRobot(),
        m_modelTasks(m_modelRobot),
        m_realRobot(realRobot)
    {
        m_modelRobot.initialize(m_modelState);
    }

    const Vec3& getTarget() const {return m_modelTasks.getTarget();}
    Vec3& updTarget() {return m_modelTasks.updTarget();}
    void setTarget(Vec3 pos) {m_modelTasks.setTarget(pos);}

    void toggleGravityComp() {m_modelTasks.toggleGravityComp();}
    void toggleTask() {m_modelTasks.toggleTask();}

    bool isGravityCompensationOn() const 
    {   return m_modelTasks.isGravityCompensationOn(); }

    // This method calculates the needed control torques and adds them into
    // the given mobilityForces Vector which will be applied to the real UR10.
    // The supplied State is from the real UR10 and will be used to read its
    // sensors.
    void calcForce(const SimTK::State&                realState,
                   SimTK::Vector_<SimTK::SpatialVec>& bodyForces,
                   SimTK::Vector_<SimTK::Vec3>&       particleForces,
                   SimTK::Vector&                     mobilityForces) const
                   OVERRIDE_11;

    // This controller does not contribute potential energy to the system.
    Real calcPotentialEnergy(const SimTK::State& state) const OVERRIDE_11
    { return 0; }

    void calcDecorativeGeometryAndAppend(const State & state, Stage stage,
            Array_<DecorativeGeometry>& geometry) const OVERRIDE_11
    {
        if (stage != Stage::Position) return;

        geometry.push_back(DecorativeSphere(0.02)
                .setTransform(m_modelTasks.getTarget())
                .setColor(m_targetColor));

        const MobilizedBody& ee = m_realRobot.getBody(UR10::EndEffector);
        Vec3 taskPosInGround = ee.findStationLocationInGround(state,
                m_modelTasks.getTaskPointInEndEffector());
        geometry.push_back(DecorativePoint(taskPosInGround)
                .setColor(Green).setLineThickness(3));

        geometry.push_back(DecorativeText(String("Gravity compensation ")
            + (m_modelTasks.isGravityCompensationOn() ? "ON" : "OFF"))
            .setIsScreenText(true));
    }


private:
    UR10                 m_modelRobot;   // The controller's internal model.
    TasksMeasure<Vector> m_modelTasks;

    mutable State   m_modelState;   // Temporary: State for the model robot.

    const UR10&     m_realRobot;    // The "real" robot being controlled.

    static const Vec3 m_targetColor;
};

const Vec3 ReachingAndGravityCompensation::m_targetColor = Red;

//------------------------------------------------------------------------------
//                               CALC FORCE
//------------------------------------------------------------------------------
// Given sensor readings from the real robot, generate control torques for it.
void ReachingAndGravityCompensation::calcForce(
               const State&         realState,
               Vector_<SpatialVec>& bodyForces,
               Vector_<Vec3>&       particleForces,
               Vector&              mobilityForces) const
{
    // Sense the real robot and use readings to update model robot.
    // ------------------------------------------------------------
    for (int i=0; i < UR10::NumCoords; ++i) {
        const UR10::Coords coord = UR10::Coords(i);
        const Real sensedQ = m_realRobot.senseJointAngle(realState, coord);
        const Real sensedU = m_realRobot.senseJointRate(realState, coord);
        m_modelRobot.setJointAngle(m_modelState, coord, sensedQ);
        m_modelRobot.setJointRate(m_modelState, coord, sensedU);
    }

    // Optional: if real robot end effector location can be sensed, it can
    // be used to improve accuracy. Otherwise, estimate the end effector
    // location using the model robot.
    const Vec3 sensedEEPos = m_realRobot.senseEndEffectorPosition(realState);
    //m_modelTasks.setSensedEndEffectorPos(sensedEEPos);

    m_modelRobot.realize(m_modelState, Stage::Velocity);

    const Vector& tau = m_modelTasks.getValue(m_modelState);

    mobilityForces += tau;
}

//==============================================================================
//                           USER INPUT HANDLER
//==============================================================================
/// This is a periodic event handler that interrupts the simulation on a
/// regular basis to poll the InputSilo for user input.
class UserInputHandler : public PeriodicEventHandler {
public:
    UserInputHandler(Visualizer::InputSilo& silo,
            ReachingAndGravityCompensation& controller, Real interval)
        :
        PeriodicEventHandler(interval), m_silo(silo), m_controller(controller),
        m_increment(0.05) {}

    void handleEvent(State& realState, Real accuracy,
            bool& shouldTerminate) const OVERRIDE_11
    {
        while (m_silo.isAnyUserInput()) {
            unsigned key, modifiers;
            while (m_silo.takeKeyHit(key, modifiers)) {
                if (key == Visualizer::InputListener::KeyEsc) {
                    shouldTerminate = true;
                    m_silo.clear();
                    return;
                }
                if (key == 'g') {
                    m_controller.toggleGravityComp();
                    return;
                }
                if (key == 't') {
                    m_controller.toggleTask();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyRightArrow) {
                    // z coordinate goes in and out of the screen.
                    m_controller.updTarget()[XAxis] -= m_increment;
                    m_silo.clear();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyLeftArrow) {
                    m_controller.updTarget()[XAxis] += m_increment;
                    m_silo.clear();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyUpArrow) {
                    m_controller.updTarget()[ZAxis] += m_increment;
                    m_silo.clear();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyDownArrow) {
                    m_controller.updTarget()[ZAxis] -= m_increment;
                    m_silo.clear();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyPageUp) {
                    m_controller.updTarget()[YAxis] -= m_increment;
                    m_silo.clear();
                    return;
                }
                else if (key == Visualizer::InputListener::KeyPageDown) {
                    m_controller.updTarget()[YAxis] += m_increment;
                    m_silo.clear();
                    return;
                }
            }
        }
    }
private:
    Visualizer::InputSilo& m_silo;
    ReachingAndGravityCompensation& m_controller;
    const Real m_increment;
};

//==============================================================================
// OutputReporter
//==============================================================================
/// This is a periodic event handler that interrupts the simulation on a
/// regular basis so that we can display information.
class OutputReporter : public PeriodicEventReporter {
public:
    OutputReporter(const UR10& system, Real interval) :
        PeriodicEventReporter(interval), m_system(system) {}
    void handleEvent(const State& state) const OVERRIDE_11 {}
private:
    const UR10& m_system;
};

//==============================================================================
//                                  MAIN
//==============================================================================
int main(int argc, char **argv) {
  try {
    cout << "Working dir=" << Pathname::getCurrentWorkingDirectory() << endl;

    // Set some options.
    const double duration = Infinity; // seconds.

    // Create the "real" robot (the one that is being simulated).
    UR10 realRobot;

    // Add the controller.
    ReachingAndGravityCompensation* controller =
        new ReachingAndGravityCompensation(realRobot);
    // Force::Custom takes ownership over controller.
    Force::Custom control(realRobot.updForceSubsystem(), controller);

    // Set up visualizer and event handlers.
    Visualizer viz(realRobot);
    viz.setShowFrameRate(true);
    viz.setShowSimTime(true);
    Visualizer::InputSilo* userInput = new Visualizer::InputSilo();
    viz.addInputListener(userInput);
    realRobot.addEventHandler(
            new UserInputHandler(*userInput, *controller, 0.05));
    realRobot.addEventReporter(new OutputReporter(realRobot, .01));
    realRobot.addEventReporter(
            new Visualizer::Reporter(viz, 1./30));

    // Display message on the screen about how to start simulation.
    DecorativeText help("Any input to start; ESC to quit.");
    help.setIsScreenText(true);
    viz.addDecoration(MobilizedBodyIndex(0), Vec3(0), help);
    help.setText("Move target w/Arrows, PageUp/Down");
    viz.addDecoration(MobilizedBodyIndex(0), Vec3(0), help);
    help.setText("g=toggle gravity comp; t=toggle task");
    viz.addDecoration(MobilizedBodyIndex(0), Vec3(0), help);

    // Initialize the real robot and other related classes.
    State s;
    realRobot.initialize(s);
    s.updQ()[UR10::LiftCoord]  = Pi/4;
    s.updQ()[UR10::ElbowCoord] = Pi/2;
    RungeKuttaMersonIntegrator integ(realRobot);
    //SemiExplicitEuler2Integrator integ(realRobot);
    integ.setAccuracy(0.001);
    TimeStepper ts(realRobot, integ);
    ts.initialize(s);
    viz.report(ts.getState());

    userInput->waitForAnyUserInput();
    userInput->clear();

    const double startCPU  = cpuTime(), startTime = realTime();

    // Simulate.
    ts.stepTo(duration);

    std::cout << "CPU time: " << cpuTime() - startCPU << " seconds. "
                << "Real time: " << realTime() - startTime << " seconds."
                << std::endl;

  } catch (const std::exception& e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
