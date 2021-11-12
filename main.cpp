#include "contiguous_line_filter.h"

int main(int argc, char *argv[])
{
	// Set parameters
	const std::string weightsFilePrefix = "weights/contiguous_line_weights_";
	const int kernelSize = 11;
	const int kernelRuns = 1;
	const int kernelSpan = 5;

	// Load input data
	cv::Mat img = cv::imread("test_input.png", 0);

	// Run local thresholding filter
	auto contiguousLineFilter = ContiguousLineFilter(weightsFilePrefix, kernelSize, kernelRuns, kernelSpan);
	cv::Mat filtered_img = contiguousLineFilter.run(img);

	// Save output image
	cv::imwrite("build/output/filtered_img.png", filtered_img);

	return 0;
}
