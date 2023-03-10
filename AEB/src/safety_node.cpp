#include <ackermann_msgs/AckermannDriveStamped.h>
#include <math.h>
#include <nav_msgs/Odometry.h>
#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>
#include <std_msgs/Bool.h>

#include <lab2/utils.hpp>
#include <limits>

/**
 * @brief
 *  One publisher should publish to the /brake topic with an
 *  ackermann_msgs/AckermannDriveStamped brake message.
 *
 *  One publisher should publish to the /brake_bool topic with a
 *  std_msgs/Bool message.
 *
 *  You should also subscribe to the /scan topic to get the
 *  sensor_msgs/LaserScan messages and the /odom topic to get
 *  the nav_msgs/Odometry messages
 *
 *  The subscribers should use the provided odom_callback and
 *  scan_callback as callback methods
 *
 *  NOTE that the x component of the linear velocity in odom is the speed
 *
 */

/**
 * @brief The class that handles emergency braking
 *
 */
class Safety {
private:
  ros::NodeHandle n;
  double speed;
  const double deceleration;

  // ROS subscribers and publishers
  ros::Subscriber odom_subscriber;
  ros::Subscriber scan_subscriber;
  ros::Publisher publish_bool;
  ros::Publisher publish_ackermann;
    double v_x;
    double v_y;

public:
  Safety()
      : deceleration(8.26)  // F1/10 racing car deceleration: 8.26 m/s^2
  {
    n     = ros::NodeHandle();
    speed = 0.0;

    // create ROS subscribers and publishers
    odom_subscriber = n.subscribe("/odom", 1, &Safety::odom_callback, this);
    scan_subscriber = n.subscribe("/scan", 1, &Safety::scan_callback, this);
    publish_bool    = n.advertise<std_msgs::Bool>("/brake_bool", 1);
    publish_ackermann =
        n.advertise<ackermann_msgs::AckermannDriveStamped>("/brake", 1);
  }

  void odom_callback(const nav_msgs::Odometry::ConstPtr &odom_msg) {
    // update current speed
    v_x = odom_msg->twist.twist.linear.x;
    v_y = odom_msg->twist.twist.linear.y;
    geometry_msgs::Vector3 velocity = odom_msg->twist.twist.linear;
    speed = lab2_utils::vecLength(velocity);
  }

  void scan_callback(const sensor_msgs::LaserScan::ConstPtr &scan_msg) {
    unsigned int N = scan_msg->ranges.size();
    double TTC_threshold = 0.4;
    double min_TTC = 100;
    for (unsigned int i = 0; i < N; ++i) {
      double range = scan_msg->ranges[i];
      if (range > scan_msg->range_max || range < scan_msg->range_min) continue;

      double angle_to_heading = scan_msg->angle_min + i * scan_msg->angle_increment;
      double projected_speed = std::max(-1 * speed * std::cos(angle_to_heading), 0.0);
      double distance_derivative = cos(angle_to_heading) * v_x + sin(angle_to_heading) * v_y;

      // calculate TTC
      if (distance_derivative > 0 && range/ distance_derivative < min_TTC) min_TTC = range / distance_derivative;
      

      // publish drive/brake message
      if (min_TTC <= TTC_threshold) {
        std_msgs::Bool msg_bool;
        msg_bool.data = true;

        ackermann_msgs::AckermannDriveStamped msg_brake;
        msg_brake.drive.speed = 0.0f;

        publish_bool.publish(msg_bool);
        publish_ackermann.publish(msg_brake);

        ROS_INFO("Emergency brake message sent");
      }
    }
  }
};

int main(int argc, char **argv) {
  ros::init(argc, argv, "safety_node");
  Safety sn;
  ros::spin();
  return 0;
}