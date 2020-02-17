/* ************************************************************************** */
/*																			*/
/*														:::	:::::::: */
/* main.cpp										 :+:	:+:	:+: */
/*													+:+ +:+		 +:+	 */
/* By: roduquen <marvin@42.fr>					+#++:+	 +#+		*/
/*												+#+#+#+#+#+ +#+		 */
/* Created: 2020/02/10 12:57:10 by roduquen		#+#	#+#			 */
/* Updated: 2020/02/10 15:13:24 by roduquen ### ########.fr */
/*																			*/
/* ************************************************************************** */

#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define MAX_CORNERS			200
#define QUALITY_LEVEL		0.01
#define MIN_DISTANCE		30
#define SMOOTHING_RADIUS	1

struct TransformParam
{
	TransformParam() {}

	TransformParam(double _dx, double _dy, double _da)
	{
		dx = _dx;
		dy = _dy;
		da = _da;
	}

	double dx;
	double dy;
	double da; // angle

	void getTransform(Mat &T)
	{
		// Reconstruct transformation matrix accordingly to new values
		T.at<double>(0,0) = cos(da);
		T.at<double>(0,1) = -sin(da);
		T.at<double>(1,0) = sin(da);
		T.at<double>(1,1) = cos(da);

		T.at<double>(0,2) = dx;
		T.at<double>(1,2) = dy;
	}
};

struct Trajectory
{
	Trajectory() {}
	Trajectory(double _x, double _y, double _a) {
		x = _x;
		y = _y;
		a = _a;
	}

	double x;
	double y;
	double a; // angle
};

vector<Trajectory> cumsum(vector<TransformParam> &transforms)
{
	vector <Trajectory> trajectory; // trajectory at all frames
	// Accumulated frame to frame transform
	double a = 0, x = 0, y = 0;

	for(size_t i = 0; i < transforms.size(); i++)
	{
		x += transforms[i].dx;
		y += transforms[i].dy;
		a += transforms[i].da;

		trajectory.push_back(Trajectory(x,y,a));
	}

	return (trajectory);
}

vector <Trajectory> smooth(vector <Trajectory> &trajectory, int radius)
{
	vector <Trajectory> smoothed_trajectory;
	for(size_t i = 0; i < trajectory.size(); i++)
	{
		double	sum_x = 0, sum_y = 0, sum_a = 0;
		int		count = 0;

		for(int j = -radius; j <= radius; j++)
		{
			if(i + j >= 0 && i + j < trajectory.size())
			{
				sum_x += trajectory[i + j].x;
				sum_y += trajectory[i + j].y;
				sum_a += trajectory[i + j].a;
				count++;
			}
		}

		double avg_a = sum_a / count;
		double avg_x = sum_x / count;
		double avg_y = sum_y / count;
		smoothed_trajectory.push_back(Trajectory(avg_x, avg_y, avg_a));
	}

	return (smoothed_trajectory);
}
void fixBorder(Mat &frame_stabilized)
{
	Mat T = getRotationMatrix2D(Point2f(frame_stabilized.cols / 2, frame_stabilized.rows / 2), 0, 1.04);
	warpAffine(frame_stabilized, frame_stabilized, T, frame_stabilized.size());
}

int			main(int ac, char **av)
{
	(void)ac;

	// Initialization of matrixes
	Mat				current, current_frame, current_gray;
	Mat				prev, prev_frame, prev_gray;

	// Initialization of video
	VideoCapture	capture(av[1]);
	int				nbr_frame = int(capture.get(CAP_PROP_FRAME_COUNT));
	int				width = int(capture.get(CAP_PROP_FRAME_WIDTH));
	int				height = int(capture.get(CAP_PROP_FRAME_HEIGHT));
	double			fps = capture.get(CAP_PROP_FPS);

	// Initialization of output video
	VideoWriter out("out.avi", VideoWriter::fourcc('M','J','P','G'), fps, Size(2 * width, height));

	// Copy the first frame of the video
	capture >> prev_frame;

	// Convert the frame to grayscale
	cvtColor(prev_frame, prev_gray, COLOR_BGR2GRAY);

	// Create the vector for transformation matrixes
	vector <TransformParam> transforms;

	cout << "nbr_frame = " << nbr_frame << endl;
	// Loop over every frames
	for (int i = 0; i < nbr_frame - 1; i++)
	{
		// Initialization of vector for points in 2d frame
		vector <Point2f>	current_pts, prev_pts;
		Mat					last_T;

		// Tracking of the best points to track in the actual frame
		goodFeaturesToTrack(prev_gray, prev_pts, MAX_CORNERS, QUALITY_LEVEL, MIN_DISTANCE);

		// Copy the next frame of the video
		capture >> current_frame;

		// Convert the frame to grayscale
		cvtColor(current_frame, current_gray, COLOR_BGR2GRAY);

		// Calculate the optical flow
		vector <uchar> status;
		vector <float> err;
		calcOpticalFlowPyrLK(prev_gray, current_gray, prev_pts, current_pts, status, err);

		// Filter only valid points
		auto prev_it = prev_pts.begin();
		auto current_it = current_pts.begin();
		for(size_t k = 0; k < status.size(); k++)
		{
			if (status[k])
			{
				prev_it++;
				current_it++;
			}
			else
			{
				prev_it = prev_pts.erase(prev_it);
				current_it = current_pts.erase(current_it);
			}
		}

		// Find transformation matrix
		Mat T = estimateAffine2D(prev_pts, current_pts);

		// In rare cases no transform is found.
		// We'll just use the last known good transform.
		if (T.data == NULL)
			last_T.copyTo(T);
		T.copyTo(last_T);

		// Extract traslation
		double dx = T.at<double>(0,2);
		double dy = T.at<double>(1,2);

		// Extract rotation angle
		double da = atan2(T.at<double>(1,0), T.at<double>(0,0));

		// Store transformation
		transforms.push_back(TransformParam(dx, dy, da));

		// Move to next frame
		current_gray.copyTo(prev_gray);

		// Print to see result
		cout << "Frame: " << i << "/" << nbr_frame - 1 << " -Tracked points : " << prev_pts.size() << endl;
	}
	vector <Trajectory> trajectory = cumsum(transforms);
	vector <Trajectory> smoothed_trajectory = smooth(trajectory, SMOOTHING_RADIUS);

	vector <TransformParam> transforms_smooth;

	for(size_t i = 0; i < transforms.size(); i++)
	{
		// Calculate difference in smoothed_trajectory and trajectory
		double diff_x = smoothed_trajectory[i].x - trajectory[i].x;
		double diff_y = smoothed_trajectory[i].y - trajectory[i].y;
		double diff_a = smoothed_trajectory[i].a - trajectory[i].a;

		// Calculate newer transformation array
		double dx = transforms[i].dx + diff_x;
		double dy = transforms[i].dy + diff_y;
		double da = transforms[i].da + diff_a;

		transforms_smooth.push_back(TransformParam(dx, dy, da));
	}
	capture.set(CAP_PROP_POS_FRAMES, 1);
	Mat T(2,3,CV_64F);
	Mat frame, frame_stabilized, frame_out;


	for(int i = 0; i < nbr_frame - 2; i++)
	{
		bool success = capture.read(frame);
		if (!success)
			break;
		// Extract transform from translation and rotation angle.
		transforms_smooth[i].getTransform(T);
		// Apply affine wrapping to the given frame
		warpAffine(frame, frame_stabilized, T, frame.size());
		// Scale image to remove black border artifact
		fixBorder(frame_stabilized);
		// Now draw the original and stabilised side by side for coolness
		hconcat(frame, frame_stabilized, frame_out);
		// If the image is too big, resize it.
//		if (frame_out.cols > 1920)
//			resize(frame_out, frame_out, Size(frame_out.cols / 2, frame_out.rows / 2));
		imshow("Before and After", frame_out);
		out.write(frame_out);
		waitKey(20);
	}
}
