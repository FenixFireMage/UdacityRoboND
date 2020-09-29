//#include <termios.h>    /* For use in keypress detection */
//#include <unistd.h>     /* For use in keypress detection */
//#include <fcntl.h>      /* For use in keypress detection */
//#include <sys/select.h> /* For use in keypress detection */
//#include <stropts.h>    /* For use in keypress detection */
//#include <time.h> /* Allows check of function process time. clock_t, clock, CLOCKS_PER_SEC */
//#include <math.h> /* Allows check of function process time. sqrt */
#include <iostream> /* printf */
// using namespace std;

#include "ros/ros.h"
#include "ball_chaser/DriveToTarget.h"
#include <sensor_msgs/Image.h>

// Define a global client that can request services
ros::ServiceClient client;

// This function calls the command_robot service to drive the robot in the specified direction
void drive_robot(float lin_x, float ang_z)
{
    // TODO: Request a service and pass the velocities to it to drive the robot
    ROS_INFO_STREAM("Drive the robot: ");

    //Request the velocites to drive the robot
    ball_chaser::DriveToTarget srv;
    srv.request.linear_x = lin_x;
    srv.request.angular_z = ang_z;

    // Call the safe_move service and pass the requested joint angles
    if (!client.call(srv))
        ROS_ERROR("Failed to call service DriveToTarget");
}

// From https://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html this function allows detection of a keypress to allow user to choose precision level
//int kbhit(void)
//{
//  struct termios oldt, newt;
//  int ch;
//  int oldf;

//  tcgetattr(STDIN_FILENO, &oldt);
//  newt = oldt;
//  newt.c_lflag &= ~(ICANON | ECHO);
//  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
//  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
//  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

//  ch = getchar();

//  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
//  fcntl(STDIN_FILENO, F_SETFL, oldf);

//  if(ch != EOF)
//  {
//    ungetc(ch, stdin);
//    return 1;
//  }

//  return 0;
//}

// This callback function continuously executes and reads the image data
void process_image_callback(const sensor_msgs::Image img)
{
    // According to https://docs.ros.org/indigo/api/sensor_msgs/html/msg/Image.html the request message includes
    // unsigned32 height         # image height, that is, number of rows
    // unsigned32 width          # image width, that is, number of columns
    // unsigned32 step           # Full row length in bytes
    // unsigned8[] data          # actual matrix data, size is (step * rows)
    // so r11g11b11 r12g12b12 r1wg1wb1w
    //    r21g21b21 r22g22b22 r2wg2wb2w
    //... |                           |
    //    rh1gh1bh1 rh2gh2bh2 rhwghwbhw
    printf("\n");
    ROS_INFO("Image Message received: height: %u, width: %u, step: %u", img.height, img.width, img.step); // 800, 800, 2400

    unsigned height_index;                                 //Row position within the image
    unsigned width_index;                                  //Column position within the image
    unsigned data_index;                                   // Position in the data array
    unsigned data_size = img.step * img.height;            // size is (step * rows) = 1920000
    unsigned left_and_forward_divider = img.step / 3;      // 800
    unsigned forward_and_right_divider = 2 * img.step / 3; // 1600

    ROS_INFO("l_and_f_divider: %u & f_and_r_divider: %u", left_and_forward_divider, forward_and_right_divider);

    int white_pixel = 255;
    // TODO: Loop through each pixel in the image and check if there's a bright white one

    /* Note there are two possible ways for detecting the ball either by
    ** low precision: checks if there's a single bright white pixel or by
    ** high precision: checks for every bright white pixel that makes up the whole ball which could be in 2 of the 3 image locations.
    */
    enum binary_flag // For use in switches
    {
        no = 0,
        yes = 1
    } high_precision,
        found_white_pixel, ball_found;
    int ball_volume_in_the_left = 0;
    int ball_volume_in_forward = 0;
    int ball_volume_in_the_right = 0;

    /*
    ** I thus attempted to include both precisions with a timed user option.
    */
    //    string str;
    //    ROS_INFO("Do you want 'high' or 'low' precision?");
    //    bool user_input = false;
    //    int endTime = clock() + 5000;
    //    while (!kbhit() && !user_input && clock() < endTime)
    //    {
    //        printf(".");
    //        fflush(stdout);
    //        usleep(1000);

    //        cin >> str;
    //        if (clock() < endTime && (str == "high" || str == "low"))
    //            user_input = true;
    //    }

    //    if (kbhit)
    //    {
    //        getline(cin, str);
    //        if (str == "high")
    //        {
    //            high_precision = yes;
    //        }
    //    }
    //    else
    high_precision = no;
    /*
    * Since I can not currently get a timed keyboard selection input for the user to work,
	* I am just manually switching commentation between the above and below variable assignments.
    */
    //        high_precision = yes;

    // clock_t t;

    //    t = clock(); //Start timer for loop
    for (data_index = 0; data_index < data_size; data_index += 3)
    {
        width_index = data_index % img.step;
        height_index = (data_index - width_index) / img.step;
        // ROS_INFO("data_index: %u, height_index: %u & width_index: %u", data_index, height_index, width_index);

        unsigned r = img.data[data_index];
        unsigned g = img.data[data_index + 1];
        unsigned b = img.data[data_index + 2];

        found_white_pixel = (r == white_pixel && g == white_pixel && b == white_pixel) ? yes : no;
        if (found_white_pixel==yes)
        {
            ball_found = yes;
            // ROS_INFO("data_index: %u, height_index: %u & width_index: %u", data_index, height_index, width_index);
            switch (high_precision)
            {
            case yes: // For high precision the location of each pixel which makes up the ball needs to be counted
                if (width_index < left_and_forward_divider)
                {
                    ball_volume_in_the_left += 1;
                }
                else if (width_index > forward_and_right_divider)
                {
                    ball_volume_in_the_right += 1;
                }
                else
                {
                    ball_volume_in_forward += 1;
                }
                break;
            default:
                goto end_check; // Break out of the loop
            }
        }
    }
end_check:
    //    t = clock() - t; //End timer for loop
    //    ROS_INFO("It took me %ld clicks (%f seconds).", t, ((float)t) / CLOCKS_PER_SEC); // Time that the loop took
    // ROS_INFO("Ballfound:%d , ball_volume_in_the_left:%d , ball_volume_in_the_right:%d , ball_volume_in_forward:%d.", (int)ball_found, ball_volume_in_the_left, ball_volume_in_the_right, ball_volume_in_forward);

    float left_velocities[] = {0.0, 0.5};
    float forward_velocities[] = {0.5, 0.0};
    float right_velocities[] = {0.0, -0.5};
    float stop[] = {0.0, 0.0};

    // Then, identify if this pixel falls in the left, mid, or right side of the image
    // Depending on the white ball position, call the drive_bot function and pass velocities to it
    switch (ball_found)
    {
    case yes: // if (ball_found)
        switch (high_precision)
        {
        case yes: // check if precision is high
            if (ball_volume_in_the_left > ball_volume_in_forward && ball_volume_in_the_left > ball_volume_in_the_right) // Request a move left
            {
            ROS_INFO("Driving left");
            drive_robot(left_velocities[0], left_velocities[1]);
            }
            else if (ball_volume_in_the_right > ball_volume_in_forward && ball_volume_in_the_right > ball_volume_in_the_left) // Request a move right
            {
            ROS_INFO("Driving right");
            drive_robot(right_velocities[0], right_velocities[1]);
            }
            else // Request a move forward
            {
            ROS_INFO("Driving forward");
            drive_robot(forward_velocities[0], forward_velocities[1]);
            }
            break;
        default: // assume precision is low
            if (width_index < left_and_forward_divider) // Request a move left
            {
            ROS_INFO("Driving left");
            drive_robot(left_velocities[0], left_velocities[1]);
            }
            else if (width_index > forward_and_right_divider) // Request a move right
            {
            ROS_INFO("Driving right");
            drive_robot(right_velocities[0], right_velocities[1]);
            }
            else // Request a move forward
            {
            ROS_INFO("Driving forward");
            drive_robot(forward_velocities[0], forward_velocities[1]);
            }
            break;
        }
        break;
    // Request a stop when there's no white ball seen by the camera
    default:
        ROS_INFO("Driving stopped");
        drive_robot(stop[0], stop[1]);
        break;
    }
}

int main(int argc, char **argv)
{
    // Initialize the process_image node and create a handle to it
    ros::init(argc, argv, "process_image");
    ros::NodeHandle n;

    // Define a client service capable of requesting services from command_robot
    client = n.serviceClient<ball_chaser::DriveToTarget>("/ball_chaser/command_robot");

    // Subscribe to /camera/rgb/image_raw topic to read the image data inside the process_image_callback function
    ros::Subscriber sub1 = n.subscribe("/camera/rgb/image_raw", 10, process_image_callback);

    // Handle ROS communication events
    ros::spin();

    return 0;
}
