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

#include "kinematic_maps/CoM.h"

REGISTER_TASKMAP_TYPE("CoM", exotica::CoM);

namespace exotica
{
  CoM::CoM()
      : initialised_(false)
  {
    marker_offset_ = KDL::Frame::Identity();
  }

  CoM::~CoM()
  {
    //TODO
  }

  EReturn CoM::update(Eigen::VectorXdRefConst x, const int t)
  {
    if (!isRegistered(t) || !getEffReferences())
    {
      INDICATE_FAILURE
      ;
      return FAILURE;
    }
    static bool fistTime = true;
    if (fistTime)
    {
      if (!changeEffToCoM())
      {
        INDICATE_FAILURE
        ;
        return FAILURE;
      }
      fistTime = false;
    }
    if (initialised_)
    {
      if (offset_callback_) offset_callback_(this, x, t);

      if (computeForwardMap(t))
      {
        if (updateJacobian_)
        {
          if (computeJacobian(t))
          {
            return SUCCESS;
          }
          else
          {
            INDICATE_FAILURE
            return FAILURE;
          }
        }
        else
        {
          return SUCCESS;
        }
      }
      else
      {
        INDICATE_FAILURE
        return FAILURE;
      }
    }
    else
    {
      INDICATE_FAILURE
      return MMB_NIN;
    }
  }

  EReturn CoM::taskSpaceDim(int & task_dim)
  {
    if (initialised_)
    {
      task_dim = dim_;
      return SUCCESS;
    }
    else
    {
      INDICATE_FAILURE
      ;
      return MMB_NIN;
    }
  }
  bool CoM::computeForwardMap(int t)
  {
    if (!initialised_)
    {
      INDICATE_FAILURE
      ;
      return false;
    }

    int N = mass_.rows(), i;
    KDL::Vector com;
    double M = mass_.sum();
    KDL::Frame root_tf = scene_->getRobotRootWorldTransform();
    root_tf.M = KDL::Rotation::Identity();
    for (i = 0; i < N; i++)
    {
      KDL::Frame tmp_frame(
          KDL::Vector(EFFPHI(3 * i), EFFPHI(3 * i + 1), EFFPHI(3 * i + 2)));
      tmp_frame = tmp_frame * (root_tf.Inverse());
      com = com + mass_[i] * tmp_frame.p;
      if (debug_->data)
      {
        geometry_msgs::Point tmp;
        tmp_frame = marker_offset_ * tmp_frame;
        tmp.x = tmp_frame.p.data[0];
        tmp.y = tmp_frame.p.data[1];
        tmp.z = tmp_frame.p.data[2];
        com_marker_.points[i] = tmp;
      }
    }

    com = com / M;

    PHI.setZero();
    for (int i = 0; i < dim_; i++)
    {
      if (fabs(com[i]) > fabs(bounds_->data[2 * i]))
      {
        PHI(i) = com[i];
      }
    }
    if (debug_->data)
    {
      KDL::Vector tmp_frame = marker_offset_ * com;
      COM_marker_.pose.position.x = tmp_frame[0];
      COM_marker_.pose.position.y = tmp_frame[1];
      COM_marker_.pose.position.z = tmp_frame[2];

      COM_marker_.header.stamp = com_marker_.header.stamp =
          goal_marker_.header.stamp = ros::Time::now();
      com_pub_.publish(com_marker_);
      COM_pub_.publish(COM_marker_);
      goal_pub_.publish(goal_marker_);
    }
    return true;
  }

  bool CoM::computeJacobian(int t)
  {
    if (!initialised_)
    {
      INDICATE_FAILURE
      ;
      return false;
    }

    JAC.setZero();
    for (int i = 0; i < mass_.size(); i++)
    {
      JAC += mass_[i] / mass_.sum()
          * EFFJAC.block(3 * i, 0, dim_, EFFJAC.cols());
    }
    return true;
  }

  EReturn CoM::initDerived(tinyxml2::XMLHandle & handle)
  {
    tinyxml2::XMLHandle tmp_handle = handle.FirstChildElement("EnableZ");
    server_->registerParam<std_msgs::Bool>(ns_, tmp_handle, enable_z_);
    if (enable_z_->data)
      dim_ = 3;
    else
      dim_ = 2;
    tmp_handle = handle.FirstChildElement("Bounds");
    if (!ok(server_->registerParam<exotica::Vector>(ns_, tmp_handle, bounds_)))
    {
      bounds_->data.resize(dim_);
      for (int i = 0; i < dim_; i++)
        bounds_->data[i] = 0;
    }
    tmp_handle = handle.FirstChildElement("Debug");
    server_->registerParam<std_msgs::Bool>(ns_, tmp_handle, debug_);
    if (debug_->data)
    {
      com_marker_.points.resize(cog_.size());
      com_marker_.type = visualization_msgs::Marker::SPHERE_LIST;
      com_marker_.color.a = .7;
      com_marker_.color.r = 0.5;
      com_marker_.color.g = 0;
      com_marker_.color.b = 0;
      com_marker_.scale.x = com_marker_.scale.y = com_marker_.scale.z = .02;
      com_marker_.action = visualization_msgs::Marker::ADD;

      COM_marker_.type = visualization_msgs::Marker::CYLINDER;
      COM_marker_.color.a = 1;
      COM_marker_.color.r = 1;
      COM_marker_.color.g = 0;
      COM_marker_.color.b = 0;
      COM_marker_.scale.x = COM_marker_.scale.y = .15;
      COM_marker_.scale.z = .02;
      COM_marker_.action = visualization_msgs::Marker::ADD;

      goal_marker_ = COM_marker_;
      goal_marker_.color.r = 0;
      goal_marker_.color.g = 1;

      com_marker_.header.frame_id = COM_marker_.header.frame_id =
          goal_marker_.header.frame_id = scene_->getRootName();
    }

    com_pub_ = server_->advertise<visualization_msgs::Marker>(
        object_name_ + "coms_marker", 1);
    COM_pub_ = server_->advertise<visualization_msgs::Marker>(
        object_name_ + "COM_marker", 1);
    goal_pub_ = server_->advertise<visualization_msgs::Marker>(
        object_name_ + "goal_marker", 1);
    initialised_ = true;
    return SUCCESS;
  }

  bool CoM::changeEffToCoM()
  {
    std::vector<std::string> names;

    if (!ok(
        scene_->getCoMProperties(object_name_, names, mass_, cog_, tip_pose_,
            base_pose_)))
    {
      INDICATE_FAILURE
      ;
      return false;
    }
    std::vector<KDL::Frame> com_offs;
    int N = mass_.size(), i;
    com_offs.resize(N);
    for (i = 0; i < N; i++)
    {
      com_offs[i] = tip_pose_[i].Inverse() * base_pose_[i]
          * KDL::Frame(cog_[i]);
    }

    if (!ok(scene_->updateEndEffectors(object_name_, com_offs)))
    {
      INDICATE_FAILURE
      ;
      return false;
    }
    if (debug_->data)
    {
      com_marker_.points.resize(cog_.size());
    }

    if (!getEffReferences())
    {
      INDICATE_FAILURE
      ;
      return false;
    }
    return true;
  }

  EReturn CoM::setOffsetCallback(
      boost::function<void(CoM*, Eigen::VectorXdRefConst, int)> offset_callback)
  {
    offset_callback_ = offset_callback;
    return SUCCESS;
  }

  EReturn CoM::setOffset(bool left, const KDL::Frame & offset)
  {
    if (debug_->data)
    {

      if (left)
      {
        com_marker_.header.frame_id = "l_sole";
        COM_marker_.header.frame_id = "l_sole";
        goal_marker_.header.frame_id = "l_sole";
      }
      else
      {
        com_marker_.header.frame_id = "r_sole";
        COM_marker_.header.frame_id = "r_sole";
        goal_marker_.header.frame_id = "r_sole";
      }
      marker_offset_ = offset.Inverse();
    }
    return SUCCESS;
  }

  void CoM::checkGoal(const Eigen::Vector3d & goal_)
  {
    if (debug_->data)
    {
      KDL::Vector goal = marker_offset_
          * KDL::Vector(goal_(0), goal_(1), goal_(2));
      goal_marker_.pose.position.x = goal[0];
      goal_marker_.pose.position.y = goal[1];
      if (!enable_z_->data)
        goal_marker_.pose.position.z = 0;
      else
        goal_marker_.pose.position.z = goal[2];
    }
  }
}
