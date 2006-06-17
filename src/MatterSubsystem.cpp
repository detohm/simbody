/* Copyright (c) 2006 Stanford University and Michael Sherman.
 * Contributors:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**@file
 *
 * Implementation of MatterSubsystem, a still-abstract Subsystem.
 */

#include "simbody/internal/common.h"
#include "simbody/internal/MatterSubsystem.h"

#include "MatterSubsystemRep.h"

namespace SimTK {

    /////////////////////
    // MatterSubsystem //
    /////////////////////

// Default constructor is inline and creates an empty handle.
// Default copy & assignment just copy the parent class.
// Default destructor destructs the parent class.

void MatterSubsystem::setForceSubsystemIndex(int subsys) {
    updRep().setForceSubsystemIndex(subsys);
}
int MatterSubsystem::getForceSubsystemIndex() const {
    return getRep().getForceSubsystemIndex();
}

/*static*/ bool 
MatterSubsystem::isInstanceOf(const Subsystem& s) {
    return MatterSubsystemRep::isA(s.getRep());
}
/*static*/ const MatterSubsystem&
MatterSubsystem::downcast(const Subsystem& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const MatterSubsystem&>(s);
}
/*static*/ MatterSubsystem&
MatterSubsystem::updDowncast(Subsystem& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<MatterSubsystem&>(s);
}

const MatterSubsystemRep& 
MatterSubsystem::getRep() const {
    return dynamic_cast<const MatterSubsystemRep&>(*rep);
}
MatterSubsystemRep&       
MatterSubsystem::updRep() {
    return dynamic_cast<MatterSubsystemRep&>(*rep);
}

int MatterSubsystem::getNBodies() const {
    return getRep().getNBodies();
}
int MatterSubsystem::getNMobilities() const {
    return getRep().getNMobilities();
}
int MatterSubsystem::getNConstraints() const {
    return MatterSubsystemRep::downcast(*rep).getNConstraints();
}
int MatterSubsystem::getParent(int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getParent(bodyNum); 
}
Array<int> 
MatterSubsystem::getChildren(int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getChildren(bodyNum); 
}
const Transform&  
MatterSubsystem::getJointFrame(const State& s, int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getJointFrame(s, bodyNum); 
}
const Transform& 
MatterSubsystem::getJointFrameOnParent(const State& s, int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getJointFrameOnParent(s, bodyNum); 
}
const Vec3&  
MatterSubsystem::getBodyCenterOfMass(const State& s, int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getBodyCenterOfMass(s,bodyNum); 
}
const Transform& 
MatterSubsystem::getBodyConfiguration(const State& s, int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getBodyConfiguration(s,bodyNum); 
}
const SpatialVec& 
MatterSubsystem::getBodyVelocity(const State& s, int bodyNum) const { 
    return MatterSubsystemRep::downcast(*rep).getBodyVelocity(s,bodyNum); 
}

void MatterSubsystem::addInGravity(const State& s, const Vec3& g, 
                                   Vector_<SpatialVec>& bodyForces) const {
    getRep().addInGravity(s,g,bodyForces);
}
void MatterSubsystem::addInPointForce(const State& s, int body, const Vec3& stationInB, 
                                      const Vec3& forceInG, Vector_<SpatialVec>& bodyForces) const {
    getRep().addInPointForce(s,body,stationInB,forceInG,bodyForces); 
}
void MatterSubsystem::addInBodyTorque(const State& s, int body, const Vec3& torqueInG,
                                      Vector_<SpatialVec>& bodyForces) const {
    getRep().addInBodyTorque(s,body,torqueInG,bodyForces); 
}
void MatterSubsystem::addInMobilityForce(const State& s, int body, int axis, const Real& d,
                                         Vector& mobilityForces) const { 
    getRep().addInMobilityForce(s,body,axis,d,mobilityForces); 
}

const Real&
MatterSubsystem::getJointQ(const State& s, int body, int axis) const { 
    return MatterSubsystemRep::downcast(*rep).getJointQ(s,body,axis); 
}
const Real&
MatterSubsystem::getJointU(const State& s, int body, int axis) const { 
    return MatterSubsystemRep::downcast(*rep).getJointU(s,body,axis); 
}

void MatterSubsystem::setJointQ(State& s, int body, int axis, const Real& q) const { 
    MatterSubsystemRep::downcast(*rep).setJointQ(s,body,axis,q); 
}
void MatterSubsystem::setJointU(State& s, int body, int axis, const Real& u) const { 
    MatterSubsystemRep::downcast(*rep).setJointU(s,body,axis,u); 
}


const Transform& MatterSubsystem::getMobilizerConfiguration(const State& s, int body) const { 
    return MatterSubsystemRep::downcast(*rep).getMobilizerConfiguration(s,body); 
}
const SpatialVec& MatterSubsystem::getMobilizerVelocity(const State& s, int body) const { 
    return MatterSubsystemRep::downcast(*rep).getMobilizerVelocity(s,body); 
}
void MatterSubsystem::setMobilizerConfiguration(State& s, int body, const Transform& X_JbJ) const { 
    MatterSubsystemRep::downcast(*rep).setMobilizerConfiguration(s,body,X_JbJ); 
}
void MatterSubsystem::setMobilizerVelocity(State& s, int body, const SpatialVec& V_JbJ) const { 
    MatterSubsystemRep::downcast(*rep).setMobilizerVelocity(s,body,V_JbJ); 
}


const Vector& MatterSubsystem::getQConstraintErrors(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).getQConstraintErrors(s); 
}
Real MatterSubsystem::calcQConstraintNorm(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).calcQConstraintNorm(s); 
}
const Vector& MatterSubsystem::getUConstraintErrors(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).getUConstraintErrors(s); 
}
Real MatterSubsystem::calcUConstraintNorm(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).calcUConstraintNorm(s); 
}
const Vector& MatterSubsystem::getUDotConstraintErrors(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).getUDotConstraintErrors(s); 
}
Real MatterSubsystem::calcUDotConstraintNorm(const State& s) const { 
    return MatterSubsystemRep::downcast(*rep).calcUDotConstraintNorm(s); 
}

bool MatterSubsystem::projectQConstraints(State& s, Vector& y_err, Real tol, Real targetTol) const { 
    return MatterSubsystemRep::downcast(*rep).projectQConstraints(
        s,y_err,tol,targetTol); 
}
bool MatterSubsystem::projectUConstraints(State& s, Vector& y_err, Real tol, Real targetTol) const { 
    return MatterSubsystemRep::downcast(*rep).projectUConstraints(
        s,y_err,tol,targetTol); 
}

} // namespace SimTK

