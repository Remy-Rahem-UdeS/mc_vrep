/*
 * Copyright 2015-2019 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#pragma once

#include <mc_control/mc_global_controller.h>

struct VREPSimulation;

struct VREPCLI
{
public:
  VREPCLI(mc_control::MCGlobalController & controller, VREPSimulation & vrep, bool stepByStep);

  void run();

  bool done() const;

  bool next() const;

  void play();

  inline bool stepByStep() const
  {
    return stepByStep_;
  }

  void toggleStepByStep();

  void nextStep();

private:
  mc_control::MCGlobalController & controller;
  VREPSimulation & vrep;
  bool done_ = false;
  bool next_ = false;
  bool stepByStep_ = false;
  std::map<std::string, double> externalForce_{{"fx", 0.0}, {"fy", 0.0}, {"fz", 0.0}, {"cx", 0.0}, {"cy", 0.0}, {"cz", 0.0}};
};
