/*
 * Copyright 2015-2019 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#include "vrep_cli.h"

#include "vrep_simulation.h"

#include <mc_rtc/logging.h>

#include <boost/algorithm/string/trim.hpp>

#include <iostream>
#include <sstream>
#include <string>

/* Anonymous namespace to hold the CLI functions */
namespace
{
bool open_grippers(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream &)
{
  controller.setGripperOpenPercent(controller.robot().name(), 1);
  return true;
}

bool close_grippers(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream &)
{
  controller.setGripperOpenPercent(controller.robot().name(), 0);
  return true;
}

bool set_gripper(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream & args)
{
  std::string gripper;
  std::vector<double> v;
  double tmp;
  args >> gripper;
  while(args.good())
  {
    args >> tmp;
    v.push_back(tmp);
  }
  controller.setGripperTargetQ(controller.robot().name(), gripper, v);
  return true;
}

bool get_joint_pos(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream & args)
{
  std::string jn;
  args >> jn;
  if(controller.robot().hasJoint(jn))
  {
    std::cout << jn << ": " << controller.robot().mbc().q[controller.robot().jointIndexByName(jn)][0] << std::endl;
  }
  else
  {
    std::cout << "No joint named " << jn << " in the robot" << std::endl;
  }
  return true;
}

bool GoToHalfSitPose(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream &)
{
  return controller.GoToHalfSitPose_service();
}

bool EnableController(mc_control::MCGlobalController & controller, VREPSimulation &, std::stringstream & ss)
{
  std::string controller_name;
  ss >> controller_name;
  return controller.EnableController(controller_name);
}

bool set_external_force(mc_control::MCGlobalController &, VREPSimulation & vrep, std::stringstream & args)
{
  std::string body;
  double fx, fy, fz, cx, cy, cz;
  args >> body >> cx >> cy >> cz >> fx >> fy >> fz;
  sva::ForceVecd f(Eigen::Vector3d{cx, cy, cz}, Eigen::Vector3d{fx, fy, fz});
  return vrep.setExternalForce(body, f);
}

bool remove_external_force(mc_control::MCGlobalController &, VREPSimulation & vrep, std::stringstream & args)
{
  std::string body;
  args >> body;
  return vrep.removeExternalForce(body);
}

bool apply_impact(mc_control::MCGlobalController &, VREPSimulation & vrep, std::stringstream & args)
{
  std::string body;
  double fx, fy, fz, cx, cy, cz;
  args >> body >> cx >> cy >> cz >> fx >> fy >> fz;
  sva::ForceVecd f(Eigen::Vector3d{cx, cy, cz}, Eigen::Vector3d{fx, fy, fz});
  return vrep.applyImpact(body, f);
}

std::map<std::string, std::function<bool(mc_control::MCGlobalController &, VREPSimulation &, std::stringstream &)>>
    cli_fn = {
        {"get_joint_pos",
         std::bind(&get_joint_pos, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"open_grippers",
         std::bind(&open_grippers, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"close_grippers",
         std::bind(&close_grippers, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"set_gripper", std::bind(&set_gripper, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"GoToHalfSitPose",
         std::bind(&GoToHalfSitPose, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"half_sitting",
         std::bind(&GoToHalfSitPose, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"enable_controller",
         std::bind(&EnableController, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"set_external_force",
         std::bind(&set_external_force, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"remove_external_force",
         std::bind(&remove_external_force, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        {"apply_impact",
         std::bind(&apply_impact, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)}};
} // namespace

VREPCLI::VREPCLI(mc_control::MCGlobalController & controller, VREPSimulation & vrep, bool stepByStep)
: controller(controller), vrep(vrep), stepByStep_(stepByStep)
{
}

void VREPCLI::run()
{
  while(!done_)
  {
    std::string ui;
    std::getline(std::cin, ui);
    std::stringstream ss;
    ss << ui;
    std::string token;
    ss >> token;
    if(token == "stop")
    {
      mc_rtc::log::info("Stopping simulation");
      done_ = true;
    }
    else if(token == "pause")
    {
      toggleStepByStep();
    }
    else if(token == "next" || token == "n" || token == "step" || token == "s")
    {
      next_ = true;
    }
    else if(cli_fn.count(token))
    {
      std::string rem;
      std::getline(ss, rem);
      boost::algorithm::trim(rem);
      std::stringstream ss2;
      ss2 << rem;
      bool ret = cli_fn[token](controller, vrep, ss2);
      if(!ret)
      {
        std::cerr << "Failed to invoke the previous command" << std::endl;
      }
    }
    else if(token == "open" || token == "o")
    {
      std::stringstream args;
      open_grippers(this->controller, this->vrep, args);
    }
    else if(token == "close" || token == "c")
    {
      std::stringstream args;
      close_grippers(this->controller, this->vrep, args);
    }
    else
    {
      std::cerr << "Unkwown command " << token << std::endl;
    }
  }
}

bool VREPCLI::done() const
{
  return done_;
}

bool VREPCLI::next() const
{
  return next_;
}

void VREPCLI::play()
{
  next_ = false;
}

void VREPCLI::toggleStepByStep()
{
  stepByStep_ = !stepByStep_;
}

void VREPCLI::nextStep()
{
  next_ = true;
}
