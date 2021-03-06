/*
 *      Author: Yiming Yang
 * 
 * Copyright (c) 2016, University Of Edinburgh 
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *  * Neither the name of  nor the names of its contributors may be used to 
 *    endorse or promote products derived from this software without specific 
 *    prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#ifndef EXOTICA_EXOTATIONS_TASK_MAPS_TASK_MAP_INCLUDE_KINEMATIC_MAPS_COLLISIONAVOIDANCE_H_
#define EXOTICA_EXOTATIONS_TASK_MAPS_TASK_MAP_INCLUDE_KINEMATIC_MAPS_COLLISIONAVOIDANCE_H_

//#define C_DEBUG
#include <exotica/TaskMap.h>
#include <exotica/Factory.h>
#include <exotica/Test.h>
#include <tinyxml2/tinyxml2.h>
#include <Eigen/Dense>
#include <boost/thread/mutex.hpp>
#include <visualization_msgs/Marker.h>

namespace exotica
{
  class CollisionAvoidance: public TaskMap
  {
    public:
      /*
       * \brief	Default constructor
       */
      CollisionAvoidance();

      /*
       * \brief	Destructor
       */
      virtual ~CollisionAvoidance();

      /**
       * @brief	Concrete implementation of the update method
       * @param	x	Input configuration
       * @return	Exotica return type
       */
      virtual EReturn update(Eigen::VectorXdRefConst x, const int t);

      /**
       * \brief Concrete implementation of the task-space size
       */
      virtual EReturn taskSpaceDim(int & task_dim);

      EReturn setPreUpdateCallback(
          boost::function<
              void(CollisionAvoidance*, Eigen::VectorXdRefConst, int)> pre_update_callback);
      EReturn setObsFrame(const KDL::Frame & tf);

      bool isClear();
    protected:
      /**
       * @brief	Concrete implementation of the initialisation method
       * @param	handle	XML handle
       * @return	Exotica return type
       */
      virtual EReturn initDerived(tinyxml2::XMLHandle & handle);

      void eigen2Point(const Eigen::Vector3d & eigen,
          geometry_msgs::Point & point)
      {
        point.x = eigen(0);
        point.y = eigen(1);
        point.z = eigen(2);
      }
//		private:
      //	Indicate if self collision checking is required
      EParam<std_msgs::Bool> self_;

      //	In hardconstrain mode, report failure once a collision is detected
      EParam<std_msgs::Bool> hard_;

      //	Safety range
      EParam<std_msgs::Float64> safe_range_;

      //	Indicate clear, true if current state has no obstacle in the safe range
      EParam<std_msgs::Bool> isClear_;

      //  When set to true, every pair of colliding bodies will print a warning message
      EParam<std_msgs::Bool> printWhenInCollision_;

      //	End-effector names
      std::vector<std::string> effs_;

      //	Initial end-effector offsets
      std::vector<KDL::Frame> init_offsets_;

      //	Link velocities
      Eigen::VectorXd vels_;
      Eigen::VectorXd old_eff_phi_;
      EParam<std_msgs::Bool> use_vel_;

      //	Internal kinematica solver
      exotica::KinematicTree kin_sol_;
      Eigen::MatrixXd effJac;

      boost::function<void(CollisionAvoidance*, Eigen::VectorXdRefConst, int)> pre_update_callback_;
      fcl::Transform3f obs_in_base_tf_;

      //	Visual debug
      EParam<std_msgs::Bool> visual_debug_;
      ros::Publisher close_pub_;
      ros::Publisher robot_centre_pub_;
      ros::Publisher world_centre_pub_;
      visualization_msgs::Marker close_;
      visualization_msgs::Marker robot_centre_;
      visualization_msgs::Marker world_centre_;
  };
}

#endif /* EXOTICA_EXOTATIONS_TASK_MAPS_TASK_MAP_INCLUDE_KINEMATIC_MAPS_COLLISIONAVOIDANCE_H_ */
