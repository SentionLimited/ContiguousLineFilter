#ifndef CONTIGUOUS_LINE_FILTER_H
#define CONTIGUOUS_LINE_FILTER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <assert.h>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

using T = uint8_t;
using T2 = uint16_t;
using T4 = uint32_t;

class ContiguousLineFilter
{
private:
  T4 mKernelSize = 0;
  T4 mKernelRuns = 0;
  T4 mKernelSpan = 0;
  T4 mKernelVecSize = 0;
  T mChecksum = 0;
  T4 mLUTsize = 0;
  std::vector<T> h_mKernel;
  std::vector<T> h_mKernelVec;
  std::vector<T2> h_mAcceptableVec;

public:
  ContiguousLineFilter(const std::string &weightsFilePrefix, const int kernelSize, const int kernelRuns, const int kernelSpan);

  //
  // Member methods
  //
  cv::Mat run(const cv::Mat &h_iImage);
};

#endif // CONTIGUOUS_LINE_FILTER_H
