////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2009, Benjamin Cohen, Andrew Dornbush
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     1. Redistributions of source code must retain the above copyright notice
//        this list of conditions and the following disclaimer.
//     2. Redistributions in binary form must reproduce the above copyright
//        notice, this list of conditions and the following disclaimer in the
//        documentation and/or other materials provided with the distribution.
//     3. Neither the name of the copyright holder nor the names of its
//        contributors may be used to endorse or promote products derived from
//        this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

/// \author Benjamin Cohen

#ifndef sbpl_manip_planner_interface_h
#define sbpl_manip_planner_interface_h

// standard includes
#include <chrono>
#include <map>
#include <memory>
#include <string>

// system includes
#include <Eigen/Dense>
#include <moveit_msgs/MotionPlanRequest.h>
#include <moveit_msgs/MotionPlanResponse.h>
#include <moveit_msgs/PlanningScene.h>
#include <moveit_msgs/RobotState.h>
#include <moveit_msgs/RobotTrajectory.h>
#include <ros/ros.h>
#include <sbpl/headers.h>
#include <visualization_msgs/MarkerArray.h>

// project includes
#include <sbpl_arm_planner/action_space.h>
#include <sbpl_arm_planner/collision_checker.h>
#include <sbpl_arm_planner/forward.h>
#include <sbpl_arm_planner/occupancy_grid.h>
#include <sbpl_arm_planner/planning_params.h>
#include <sbpl_arm_planner/robot_heuristic.h>
#include <sbpl_arm_planner/robot_model.h>

SBPL_CLASS_FORWARD(Heuristic);
SBPL_CLASS_FORWARD(SBPLPlanner);

namespace sbpl {
namespace manip {

class ManipLattice;

class PlannerInterface
{
public:

    PlannerInterface(
        RobotModel* robot,
        CollisionChecker* checker,
        OccupancyGrid* grid);

    ~PlannerInterface();

    bool init(const PlanningParams& params);

    bool solve(
        const moveit_msgs::PlanningScene& planning_scene,
        const moveit_msgs::MotionPlanRequest& req,
        moveit_msgs::MotionPlanResponse& res);

    bool canServiceRequest(
        const moveit_msgs::MotionPlanRequest& req,
        moveit_msgs::MotionPlanResponse& res) const;

    /// @brief Return planning statistics from the last call to solve.
    ///
    /// Possible keys to statistics include:
    ///     "initial solution planning time"
    ///     "initial epsilon"
    ///     "initial solution expansions"
    ///     "final epsilon planning time"
    ///     "final epsilon"
    ///     "solution epsilon"
    ///     "expansions"
    ///     "solution cost"
    ///
    /// @return The statistics
    std::map<std::string, double> getPlannerStats();

    /// \name Visualization
    ///@{

    visualization_msgs::MarkerArray getGoalVisualization() const;
    visualization_msgs::MarkerArray getBfsWallsVisualization() const;
    visualization_msgs::MarkerArray getBfsValuesVisualization() const;

    visualization_msgs::MarkerArray getCollisionModelTrajectoryVisualization(
        const moveit_msgs::RobotState& ref_state,
        const moveit_msgs::RobotTrajectory& traj) const;

    ///@}

protected:

    RobotModel* m_robot;
    CollisionChecker* m_checker;
    OccupancyGrid* m_grid;

    PlanningParams m_params;

    // params
    bool m_initialized;

    // planner components

    RobotPlanningSpacePtr m_lattice;
    ActionSpacePtr m_action_space;
    std::map<std::string, RobotHeuristicPtr> m_heuristics;
    SBPLPlannerPtr m_planner;

    /// \name MHA*-Specific Heuristics
    ///@{
    std::vector<Heuristic*> m_heur_vec;
    ///@}

    int m_sol_cost;

    std::string m_planner_id;

    moveit_msgs::MotionPlanRequest m_req;
    moveit_msgs::MotionPlanResponse m_res;

    bool checkConstructionArgs() const;

    // Initialize the SBPL planner and the sbpl_arm_planner environment
    bool initializePlannerAndEnvironment();

    bool checkParams(const PlanningParams& params) const;

    // Set start configuration
    bool setStart(const moveit_msgs::RobotState& state);

    // Set goal(s)
    bool setGoalPosition(const moveit_msgs::Constraints& goals);

    // use this to set a 7dof goal!
    bool setGoalConfiguration(const moveit_msgs::Constraints& goal_constraints);

    // Plan a path to a cartesian goal(s)
    bool planToPose(
        const moveit_msgs::MotionPlanRequest& req,
        moveit_msgs::MotionPlanResponse& res);
    bool planToConfiguration(
        const moveit_msgs::MotionPlanRequest& req,
        moveit_msgs::MotionPlanResponse& res);

    // Retrieve plan from sbpl
    bool plan(std::vector<RobotState>& path);

    bool extractGoalPoseFromGoalConstraints(
        const moveit_msgs::Constraints& goal_constraints,
        Eigen::Affine3d& goal_pose_out,
        Eigen::Vector3d& offset) const;

    // extract tolerance as an array of 6 doubles: x, y, z, roll, pitch, yaw
    bool extractGoalToleranceFromGoalConstraints(
        const moveit_msgs::Constraints& goal_constraints,
        double* tolerance_out);

    void clearMotionPlanResponse(
        const moveit_msgs::MotionPlanRequest& req,
        moveit_msgs::MotionPlanResponse& res) const;

    bool parsePlannerID(
        const std::string& planner_id,
        std::string& search_name,
        std::string& heuristic_name,
        std::string& graph_name) const;

    void clearGraphStateToPlannerStateMap();
    bool reinitPlanner(const std::string& planner_id);

    bool isPathValid(const std::vector<RobotState>& path) const;
    void postProcessPath(
        const std::vector<RobotState>& path,
        trajectory_msgs::JointTrajectory& traj) const;
    void convertJointVariablePathToJointTrajectory(
        const std::vector<RobotState>& path,
        trajectory_msgs::JointTrajectory& traj) const;
    void profilePath(trajectory_msgs::JointTrajectory& traj) const;
    void visualizePath(
        const moveit_msgs::RobotState& traj_start,
        const moveit_msgs::RobotTrajectory& traj) const;
};

} // namespace manip
} // namespace sbpl

#endif
