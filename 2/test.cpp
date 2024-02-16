#include <opencv2/opencv.hpp> 
#include <stdio.h>
#include <iostream>

int main(int argc, char** argv) 
{
    std::string inputFileName, outputFileName;
    std::cout << "Input path of input image" << std::endl;
    std::cin >> inputFileName;
    std::cout << "Input path of output image" << std::endl;
    std::cin >> outputFileName;

    cv::Mat inputImage;
    inputImage = cv::imread(inputFileName, 1); 
    if (!inputImage.data) { 
        printf("No image data \n"); 
        return -1; 
    }

    double alpha = 0;
    std::cout << "input alpha" << std::endl;
    std::cin >> alpha;

    cv::Vec3b whitePixel = cv::Vec3b(255, 255, 255);

    cv::Point2f center(inputImage.cols / 2.0f, inputImage.rows / 2.0f);

    // Получение матрицы поворота
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, (-1) * alpha, 1.0);
    double angleRad = alpha * CV_PI / 180.0;
    double sinAngle = std::abs(std::sin(angleRad));
    double cosAngle = std::abs(std::cos(angleRad));

    int newWidth = int(inputImage.rows * sinAngle + inputImage.cols * cosAngle);
    int newHeight = int(inputImage.cols * sinAngle + inputImage.rows * cosAngle);

    // Корректируем матрицу поворота для нового центра
    cv::Point2f newCenter(newWidth / 2.0f, newHeight / 2.0f);
    rotationMatrix.at<double>(0, 2) += newCenter.x - center.x;
    rotationMatrix.at<double>(1, 2) += newCenter.y - center.y;

    // Поворот изображения
    cv::Mat rotatedImage;
    cv::warpAffine(inputImage, rotatedImage, rotationMatrix, cv::Size(newWidth, newHeight), 
                   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    cv::imwrite(outputFileName, rotatedImage);
    return 0; 
}
