#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <queue>
#include <stdio.h>
#include <vector>

const int SHIFT = 10;

bool isWhite(cv::Mat& image, int i, int j)
{
    cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);
    return (static_cast<int>(pixel[0]) == 255
        && static_cast<int>(pixel[1]) == 255
        && static_cast<int>(pixel[2]) == 255);
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

    // Поиск внутренней точки фото на картинке.
    for (int i = SHIFT; i < inputImage.rows - SHIFT; i += SHIFT * 2) {
        for (int j = SHIFT; j < inputImage.cols - SHIFT; j += SHIFT * 2) {
            // Поиск окна, в котором нет белых пикселей.
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
            
            // Нашли внутреннюю точку, начинаем поиск границ фото.
            if (flag) {
                // С помощью BFS формируем множество пикселей фото.
                std::set<std::pair<int, int>> picture;
                std::queue<std::pair<int, int>> q;
                q.push(std::make_pair(i, j));
                picture.insert(std::make_pair(i, j));
                while (!q.empty()) {
                    auto pixel = q.front();
                    q.pop();
                    for (int y = pixel.first - 1; y < pixel.first + 2; y++) {
                        for (int x = pixel.second - 1; x < pixel.second + 2;
                             x++) {
                            if (x < 0 || y < 0 || x >= inputImage.cols
                                || y >= inputImage.rows) {
                                continue;
                            }
                            if ((picture.find(std::make_pair(y, x))
                                    == picture.end())
                                && !isWhite(inputImage, y, x)) {
                                q.push(std::make_pair(y, x));
                                picture.insert(std::make_pair(y, x));
                            }
                        }
                    }
                }

                // Поиск угловых точек фото.
                std::pair<int, int> minY = { inputImage.rows + 1, 0 },
                                    maxY = { 0, 0 };
                std::pair<int, int> minX = { 0, inputImage.cols + 1 },
                                    maxX = { 0, 0 };
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

                // Вычисление чторон прямоугольника
                double alpha;
                double AB = sqrt(static_cast<double>(
                    (minY.second - minX.second) * (minY.second - minX.second)
                    + (minY.first - minX.first) * (minY.first - minX.first)));
                double CD = sqrt(static_cast<double>(
                    (maxX.second - minY.second) * (maxX.second - minY.second)
                    + (maxX.first - minY.first) * (maxX.first - minY.first)));

                // Вычисление угла поворота
                if (AB <= CD) {
                    alpha = (atan(static_cast<double>(minX.second - minY.second)
                                / static_cast<double>(minX.first - minY.first)))
                        * 180.0 / M_PI;
                } else {
                    alpha = ((M_PI / 2
                                 + atan(static_cast<double>(
                                            minX.second - minY.second)
                                     / static_cast<double>(
                                         minX.first - minY.first)))
                        * 180.0 / M_PI);
                }

                std::cout << (-1) * alpha << "\n";

                // Обратный поворот изображения
                cv::Point2f center(
                    inputImage.cols / 2.0f, inputImage.rows / 2.0f);
                cv::Mat rotationMatrix
                    = cv::getRotationMatrix2D(center, (-1) * alpha, 1.0);
                cv::Mat rotatedImage;

                cv::warpAffine(inputImage, rotatedImage, rotationMatrix,
                    inputImage.size());
                cv::imwrite(outputFileName, rotatedImage);
                return 0;
            }
        }
    }

    return 0;
}
