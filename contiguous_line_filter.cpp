#include "contiguous_line_filter.h"

//===================================================================

///
/// Setup function
///
ContiguousLineFilter::ContiguousLineFilter(
    const std::string &weightsFilePrefix,
    const int kernelSize,
    const int kernelRuns,
    const int kernelSpan)
{

    mKernelSize = kernelSize;
    mKernelRuns = kernelRuns;
    mKernelSpan = kernelSpan;

    ///
    /// Create the kernel for the first pass
    ///

    {

        //
        // Populate the kernel with concentric values of powers of 2 (with the highest value being at
        // the center of the kernel
        //
        assert(mKernelSize <= 13 && "Contiguous line kernel size cannot be larger than 13 (due to powers of 2 and the uint8_t type)");
        assert(mKernelSize % 2 == 1 && "Contiguous line kernel size must be odd in size");

        const T4 totalKernelSize = mKernelSize * mKernelSize;
        const T4 mhKernelSize = mKernelSize / 2;

        //
        // Initialize kernel vector
        //
        h_mKernel = std::vector<T>(totalKernelSize, 0);

        //
        // Calculate vector of powers of two in descending order
        //
        T powersOfTwo[mhKernelSize + 1];
        for (int i = 0; i < mhKernelSize + 1; ++i)
            powersOfTwo[mhKernelSize - i] = std::pow(2, i);

        //
        // Concentrically populate squares (from outermost to innermost) with the elements in powersOfTwo vector
        //
        for (int i = 0; i < mhKernelSize + 1; ++i)
        {
            for (int x = i; x < mKernelSize - i; ++x)
            {
                for (int y = i; y < mKernelSize - i; ++y)
                {
                    h_mKernel[y * mKernelSize + x] = powersOfTwo[i];
                }
            }
        }

        //
        // Calculate checksum (sum of straight line going through the center of the kernel)
        //
        for (T4 i = 0; i < mKernelSize; ++i)
        {
            mChecksum += h_mKernel[mhKernelSize * mKernelSize + i];
        }

    } // scope of pass 1

    //-------------------------------------------------------------------------

    ///
    /// Create the kernel for the second pass
    ///

    {

        assert(mKernelSpan % 2 == 1 && "Contiguous line kernel span must be odd");

        mKernelVecSize = (mKernelSize - 1) * 4;

        //
        // Allocate memory for the kernel vector
        //
        h_mKernelVec = std::vector<T>(mKernelVecSize, 0);

        //
        // Read the kernel vector file
        //
        std::string weightsFile = weightsFilePrefix + std::to_string(mKernelSize) + ".dat";

        std::ifstream fin;
        fin.open(weightsFile.c_str());
        assert(fin && "Could not open the kernel vector file.");

        for (int i = 0; i < mKernelVecSize; ++i)
        {
            int tmp;
            fin >> tmp; // cannot directly read uint8_t from std::ifstream so am reading to int first
            h_mKernelVec[i] = static_cast<T>(tmp);
        }

        //
        // Create vector of acceptable products between pairs of perimeter elements
        //

        mLUTsize = mKernelVecSize * mKernelSpan / 2;

        std::vector<T2> acceptable_vec(mLUTsize);

        const int offset = mKernelSize * 2 - 2;
        const int hKernelSpan = mKernelSpan / 2;

        int counter = 0;
        for (int i = 0; i < mKernelVecSize; ++i)
        {
            int opposite_end = i + offset;

            for (int j = -hKernelSpan; j < hKernelSpan + 1; ++j)
            {
                int prod = static_cast<int>(h_mKernelVec[i]) *
                           static_cast<int>(h_mKernelVec[(opposite_end + j) % mKernelVecSize]);

                if (std::find(acceptable_vec.begin(), acceptable_vec.end(), prod) == acceptable_vec.end())
                {
                    acceptable_vec[counter] = static_cast<T2>(prod);
                    ++counter;
                }
            }
        }

        // Sort vector (not necessary, but better to visualize and compare with iPython results)
        std::sort(acceptable_vec.begin(), acceptable_vec.end());

        //
        // Allocate memory for the acceptable vector
        //
        h_mAcceptableVec = std::vector<T2>(mLUTsize);

        for (int i = 0; i < mLUTsize; ++i)
        {
            h_mAcceptableVec[i] = acceptable_vec[i];
        }

    } // scope of pass 2
}

//===================================================================

///
/// Run contiguous line filter
///
cv::Mat ContiguousLineFilter::run(
    const cv::Mat &h_iImage)
{
    // Ensure input has values 0 or 1 (not 255)
    cv::Mat clamped;
    cv::threshold(h_iImage, clamped, 1, 1, 0);

    //
    // Set half kernel size
    //
    const int mhKernelSize = mKernelSize / 2; // Note: Must be int for inner loops

    //
    // Perform straight-line filtering
    //

    cv::Mat h_pImage;
    copyMakeBorder(clamped, h_pImage, mhKernelSize, mhKernelSize, mhKernelSize, mhKernelSize, cv::BORDER_CONSTANT, 0);

    for (T4 i = mhKernelSize; i < h_pImage.cols - mhKernelSize; ++i)
    {
        for (T4 j = mhKernelSize; j < h_pImage.rows - mhKernelSize; ++j)
        {

            T2 tmp = 1;

            if (h_pImage.at<T>(j, i) > 0)
            {

                int num_pos = 0;

                // Top row (excluding right-most pixel)
                for (int k = -mhKernelSize; k < mhKernelSize; ++k)
                {
                    if (num_pos >= 2)
                        break;
                    if (h_pImage.at<T>(j - mhKernelSize, i + k) > 0)
                    {
                        tmp *= h_mKernelVec[k + mhKernelSize];
                        ++num_pos;
                    }
                }
                // Right column (excluding bottom-most pixel)
                for (int k = -mhKernelSize; k < mhKernelSize; ++k)
                {
                    if (num_pos >= 2)
                        break;
                    if (h_pImage.at<T>(j + k, i + mhKernelSize) > 0)
                    {
                        tmp *= h_mKernelVec[k + mhKernelSize + (mKernelSize - 1)];
                        ++num_pos;
                    }
                }
                // Bottom row (excluding left-most pixel)
                for (int k = -mhKernelSize; k < mhKernelSize; ++k)
                {
                    if (num_pos >= 2)
                        break;
                    if (h_pImage.at<T>(j + mhKernelSize, i - k) > 0)
                    {
                        tmp *= h_mKernelVec[k + mhKernelSize + 2 * (mKernelSize - 1)];
                        ++num_pos;
                    }
                }
                // Left column (excluding top-most pixel)
                for (int k = -mhKernelSize; k < mhKernelSize; ++k)
                {
                    if (num_pos >= 2)
                        break;
                    if (h_pImage.at<T>(j - k, i - mhKernelSize) > 0)
                    {
                        tmp *= h_mKernelVec[k + mhKernelSize + 3 * (mKernelSize - 1)];
                        ++num_pos;
                    }
                }

                if (num_pos == 2)
                {

                    bool is_acceptable = false;
                    for (int elem = 0; elem < mLUTsize; ++elem)
                    {
                        if (tmp == h_mAcceptableVec[elem])
                        {
                            is_acceptable = true;
                            break;
                        }
                    }

                    if (!is_acceptable)
                    {
                        clamped.at<T>(j - mhKernelSize, i - mhKernelSize) = 0;
                    }
                }
            }
        }
    }

    //
    // Perform contiguous-line filtering
    //

    for (int run = 0; run < mKernelRuns; ++run)
    {

        //
        // Pad input image with zeros
        //
        cv::Mat h_pImage2;
        copyMakeBorder(clamped, h_pImage2, mhKernelSize, mhKernelSize, mhKernelSize, mhKernelSize, cv::BORDER_CONSTANT, 0);

        //
        // Perform filtering
        //
        for (T4 i = mhKernelSize; i < h_pImage2.cols - mhKernelSize; ++i)
        {
            for (T4 j = mhKernelSize; j < h_pImage2.rows - mhKernelSize; ++j)
            {

                T tmp = 0;

                for (int k = -mhKernelSize; k <= mhKernelSize; ++k)
                {
                    for (int l = -mhKernelSize; l <= mhKernelSize; ++l)
                    {
                        const T4 coefPos = (k + mhKernelSize) * mKernelSize + (l + mhKernelSize);
                        tmp += h_pImage2.at<T>(j + k, i + l) * h_mKernel[coefPos];
                    }
                }

                tmp = tmp == mChecksum ? 255 : 0;
                clamped.at<T>(j - mhKernelSize, i - mhKernelSize) = tmp;
            }
        }
    }

    return clamped;
}
