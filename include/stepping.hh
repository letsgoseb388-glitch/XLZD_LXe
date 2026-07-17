#ifndef STEPPING_HH
#define STEPPING_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "construction.hh"

class MySteppingAction : public G4UserSteppingAction
{
public:
    MySteppingAction();
    ~MySteppingAction();
    virtual void UserSteppingAction(const G4Step*);
};

#endif
