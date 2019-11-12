//
// Created by rika on 10/11/2019.
//

#include <vector>
#include <numeric>
#include <algorithm>
#include "YAAPT.h"

using namespace Eigen;

void YAAPT::cmp_rate(
        const ArrayXd & phi, double fs, const Params & prm,
        int maxCands, int lagMin, int lagMax,
        ArrayXd & pitch, ArrayXd & merit)
{
    // Width of the window used in the first pass of peak picking.
    double width = prm.nccfPWidth;
    // The center of the window is defined as the peak location.
    int center = std::round(width / 2.0);

    // The threshold value used for the first pass of
    // peak picking for each frame. Any peaks found greater than this
    // are chosen for the first pass.
    double Merit_thresh1 = prm.nccfThresh1;

    // The threshold value used to limit peak searching.
    // If a peak is found at least this great, no further searching is
    // done.
    double Merit_thresh2 = prm.nccfThresh2;

    int numPeaks = 0;
    pitch.setZero(maxCands);
    merit.setZero(maxCands);

    // Find all peaks for a (lagMin to lagMax) search range.
    //  A "peak" must be higher than a specified number of points on either side.
    //  Peaks are later "cleaned" up, to retain only best peaks, i.e. peaks which
    //  do not meet certain criteria are eliminated, allowed only peaks which are
    //  a certain amplitude over the rest.

    std::vector<double> pitchvec;
    std::vector<double> meritvec;
    for (int n = lagMin - center; n <= lagMax; ++n) {
        int lag;
        double y = phi.segment(n, width).maxCoeff(&lag);
        if (lag == center && y > Merit_thresh1) {
            pitchvec.push_back(fs / static_cast<double>(n + lag - 1));
            meritvec.push_back(y);

            if (y > Merit_thresh2)
                break;
        }
    }

    numPeaks = pitchvec.size();

    // Consider the case when the number of peaks are more than the maxCands.
    // Then take only the best maxCands peaks based on the Merit values.
    std::vector<Index> idx(numPeaks);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
            [&meritvec](Index i, Index j) { return meritvec[i] > meritvec[j]; });
    merit = Map<ArrayXd>(meritvec.data(), numPeaks)(idx);
    pitch = Map<ArrayXd>(pitchvec.data(), numPeaks)(idx);
    numPeaks = std::min(numPeaks, maxCands);
    merit.conservativeResize(maxCands);
    pitch.conservativeResize(maxCands);

    // If the number of peaks in the frame are less than the maxCands, then we
    // assign null values to remainder of peak and merit values in arrays.
    if (numPeaks < maxCands) {
        pitch.tail(maxCands - numPeaks).setZero();
        merit.tail(maxCands - numPeaks).setConstant(0.001);
    }

    // Normalize merits.
    double maxMerit = merit.maxCoeff();
    if (maxMerit > 1.0) {
        merit /= maxMerit;
    }
}