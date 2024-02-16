#include <opencv2/opencv.hpp> 
#include <cmath>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <queue>

const int SHIFT = 10;

bool isWhite(cv::Mat &image, int i, int j)
{
    cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);
    return (static_cast<int>(pixel[0]) == 255 && 
            static_cast<int>(pixel[1]) == 255 &&
            static_cast<int>(pixel[2]) == 255);
}

int main(int argc, char** argv) 
{
    std::string inputFileName, outputFileName;
    std::cout << "Input path of input image" << std::endl;
    std::cin >> inputFileName;
    std::cout << "Input path of result image" << std::endl;
    std::cin >> outputFileName;

    cv::Mat inputImage;
    inputImage = cv::imread(inputFileName, 1); 
    if (!inputImage.data) { 
        printf("No image data \n"); 
        return -1; 
    }
    
    for (int i = SHIFT; i < inputImage.rows - SHIFT; i += SHIFT * 2) {
        for (int j = SHIFT; j < inputImage.cols - SHIFT; j += SHIFT * 2) {
            bool flag = true;
            for (int y = i - SHIFT; y < i + SHIFT; y++) {
                for (int x = j - SHIFT; x < j + SHIFT; x++) {
                    if (isWhite(inputImage, y, x)) {
                            flag = false;
                            break;
                        }
                }
                if (!flag) {
                    break;
                }
            }

            if (flag) {
                std::set<std::pair<int, int>> picture;
                std::queue<std::pair<int, int>> q;
                q.push(std::make_pair(i, j));
                picture.insert(std::make_pair(i, j));
                while (!q.empty()) {
                    auto pixel = q.front();
                    q.pop();
                    for (int y = pixel.first - 1; y < pixel.first + 2; y++) {
                        for (int x = pixel.second - 1; x < pixel.second + 2; x++) {
                            if (x < 0 || y < 0 || x >= inputImage.cols || y >= inputImage.rows) {
                                continue;
                            }
                            if ((picture.find(std::make_pair(y, x)) == picture.end()) && !isWhite(inputImage, y, x)) {
                                q.push(std::make_pair(y, x));
                                picture.insert(std::make_pair(y, x));
                            }
                        }
                    }
                }

                std::pair<int, int> minY = {inputImage.rows + 1, 0}, maxY = {0, 0};
                std::pair<int, int> minX = {0, inputImage.cols + 1}, maxX = {0, 0};
                for (auto k : picture) {
                    if (k.first <= minY.first) {
                        minY = k;
                    }
                    if (k.first >= maxY.first) {
                        maxY = k;
                    }
                    if (k.second <= minX.second) {
                        minX = k;
                    }
                    if (k.second >= maxX.second) {
                        maxX = k;
                    }
                }
                // std::vector<std::pair<int, int>> pictureCorners(4);
                double alpha = atan(static_cast<double>(minX.second - minY.second) / 
                                    static_cast<double>(minX.first - minY.first)) * 180.0 / M_PI;
                if (minX.first < maxX.first) {
                    alpha = 90 - (-1) * alpha;
                }
                std::cout << alpha << "\n";

                cv::Point2f center(inputImage.cols / 2.0f, inputImage.rows / 2.0f);
                cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, (-1) * alpha, 1.0);
                cv::Mat rotatedImage;

                cv::warpAffine(inputImage, rotatedImage, rotationMatrix, inputImage.size());
                cv::imwrite(outputFileName, rotatedImage);
                return 0;
            }
        }
    }
    
    return 0; 
}
