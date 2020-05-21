/*


   Copyright (c) 2020, Tharaka Ratnayake, email: tharakasachintha.17@cse.mrt.ac.lk
   All rights reserved. https://github.com/tharaka27/ImageStitcherFAST

   Some algorithms used in this code referred to:
   1. OpenCV: http://opencv.org/



   Revision history:
	  March 30th, 2020: initial version.
	  April 23th, 2020: Changed ORB implementation.
*/



#include <stdio.h>
#include <iostream> 

#include "testbench.h"
#include "ImageStitcherFAST.h"

#include "homography.h"


#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/stitching.hpp"
#include "opencv2/features2d.hpp"
#include <opencv2/calib3d.hpp>
#include "RANSAC_algo.h"
#include "ORB.h"
#include "ORBExtractor.h"
#include "ORBMatcher.h"


#include "chrono"


using namespace std;
//using namespace cv;


cv::Mat ImageStitcherFAST::calculate_h_matrix(cv::Mat gray_image1, cv::Mat gray_image2)
{
	
	//-- Step 1: Detect the keypoints using FAST Detector
	vector<cv::KeyPoint> keypoints_object, keypoints_scene;


	cv::Ptr<cv::FastFeatureDetector> detector = cv::FastFeatureDetector::create();

	
	detector->detect(gray_image1, keypoints_object);
	detector->detect(gray_image2, keypoints_scene);
	
	cout << "no of keypoints of object " << keypoints_object.size() << endl;
	cout << "no of keypoints of scene " << keypoints_scene.size() << endl;
	cout << "Detecting keypoints using FAST detector is completed" << endl << endl;

	cv::Mat descriptors_object, descriptors_scene;



	//-- Step 2: Calculate descriptors (feature vectors)
	//Mat descriptors_object, descriptors_scene;
	//Ptr<ORB> extractor = ORB::create();
	//extractor->compute(gray_image1, keypoints_object, descriptors_object);
	//extractor->compute(gray_image2, keypoints_scene, descriptors_scene);

	//cv::Ptr<TORB::ORBExtractor> de = new TORB::ORBExtractor(5000, 1.2f, 8, 31, 20); //(int nfeatures, float scaleFactor, int nlevels, int iniThFAST, int minThFAST)
	//(*de)(gray_image1, cv::Mat(), keypoints_object, descriptors_object);
	//(*de)(gray_image2, cv::Mat(), keypoints_scene, descriptors_scene);


	TORB::ORBExtractor orbextractor(1000, 1.2, 8, 31, 20);

	
	orbextractor(gray_image1, cv::Mat(), keypoints_object, descriptors_object);
	
	
	orbextractor(gray_image2, cv::Mat(), keypoints_scene, descriptors_scene);
	
	cout << "no of desciptors of object " << descriptors_object.size() << endl;
	cout << "no of desciptors of scene " << descriptors_scene.size() << endl;
	cout << "Calculating descriptors using ORB is completed" << endl << endl;


	//-- Step 3: Matching descriptor vectors using FLANN matcher
	std::vector< cv::DMatch > matches;


	//cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");
	//matcher->match(descriptors_object, descriptors_scene, matches);
	//matcher->match(descriptors_object, descriptors_scene, matches);
	//std::vector<int> matches = ORBmatcher::MatchDescriptors(descriptors1, descriptors2);

	ORBMatcher orb;


	orb.MatchDescriptors(descriptors_object, descriptors_scene, &matches);
	cout << "Matching the descriptors was successful" << endl << endl;
	

	cv::Mat H;

	double max_dist = 0; double min_dist = 100;

	//-- Quick calculation of max and min distances between keypoints 
	for (int i = 0; i < matches.size(); i++)
	{
		double dist = matches[i].distance;
		if (dist < min_dist) min_dist = dist;
		if (dist > max_dist) max_dist = dist;
	}

	printf("-- Max dist: %f \n", max_dist);
	printf("-- Min dist: %f \n", min_dist);


	// Exceptional case
	  // For the first time min_dist became 0
	  // since 3*min_distance = 0 we could not find any good match
	  // hence the default value was set to 3 if min_dis = 0

	if (min_dist < 15) {
		min_dist = 15;
	}

	//-- Use only "good" matches (i.e. whose distance is less than 3*min_dist )
	std::vector< cv::DMatch > good_matches;

	for (int i = 0; i < matches.size(); i++)
	{
		if (matches[i].distance < 3 * min_dist)
		{
			good_matches.push_back(matches[i]);
		}
	}




	//for (int i = 0; i < good_matches.size(); i++) {
	//	cout << good_matches[i].queryIdx << " trainIdx:" << good_matches[i].trainIdx << "  distance:" << good_matches[i].distance << endl;

	//}

	//cout << "Sample..........." << endl;
	//for (int i = 0; i < good_matches.size(); i++){
	//	cout << descriptors_object.row(good_matches[i].queryIdx) << endl;
	//	cout << descriptors_scene.row(good_matches[i].trainIdx) << endl;
	//	cout << good_matches[i].distance << endl;
	//}
		


	cout << good_matches.size() << endl;
	
	std::vector< cv::Point2d > obj;
	std::vector< cv::Point2d > scene;

	for (int i = 0; i < good_matches.size(); i++)
	{
		//-- Get the keypoints from the good matches
		obj.push_back(keypoints_object[good_matches[i].queryIdx].pt);
		scene.push_back(keypoints_scene[good_matches[i].trainIdx].pt);
	}

	if (obj.empty()|| scene.empty()) {
		cout << "No matches found at all....." << endl;
		throw exception();

	}
	else {
		cout << "Enough matches found" << endl;

	}




	
	cv::Mat mask;

	//H = findHomography(obj, scene, mask, cv::RANSAC);
	
	RANSAC_algo ra;

	
	struct returnRANSAC r = ra.computeHomography_RANSAC(obj, scene);
	

	return r.Hmatrix;


	

}



cv::Mat ImageStitcherFAST::stitch_image(cv::Mat image1, cv::Mat image2, cv::Mat H)
{  

	
	cv::Mat result;
	// cv::Mat result23;
	warpPerspective(image1, result, H, cv::Size(image1.cols + image2.cols, image1.rows));
	cv::Mat half(result, cv::Rect(0, 0, image2.cols, image2.rows));
	image2.copyTo(half);

	return result;
}



void ImageStitcherFAST::scale_image(cv::Mat &InputImage, cv::Mat &OutputImage, int scale_percent)
{
	int width = int(InputImage.cols * scale_percent / 100);
	int height = int(InputImage.rows * scale_percent / 100);
	cv::resize(InputImage, OutputImage, cv::Size(width, height));
}

