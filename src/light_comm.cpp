#include "light_comm.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>
#include <utility>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace
{
    // 发送端显示窗口分辨率
    constexpr int kFrameWidth = 1280;
    constexpr int kFrameHeight = 720;

    // 计算两种颜色在BGR空间的平方距离
    double colorDistanceSquared(const cv::Scalar& a, const cv::Scalar& b)
    {
        const double db = a[0] - b[0];
        const double dg = a[1] - b[1];
        const double dr = a[2] - b[2];
        return db * db + dg * dg + dr * dr;
    }

    // 在给定色板中寻找与采样色最接近的索引
    int nearestIndex(const cv::Scalar& color, const std::array<cv::Scalar, 8>& palette, int count)
    {
        int bestIndex = 0;
        double bestDistance = colorDistanceSquared(color, palette[0]);

        for (int i = 1; i < count; ++i)
        {
            const double distance = colorDistanceSquared(color, palette[i]);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    // 取图像中心区域均值颜色，降低噪声影响
    cv::Scalar sampleCenterColor(const cv::Mat& frame)
    {
        const int roiSize = 120;
        const int x = std::max(0, frame.cols / 2 - roiSize / 2);
        const int y = std::max(0, frame.rows / 2 - roiSize / 2);
        const int w = std::min(roiSize, frame.cols - x);
        const int h = std::min(roiSize, frame.rows - y);

        cv::Rect roi(x, y, w, h);
        const cv::Scalar meanColor = cv::mean(frame(roi));
        return meanColor;
    }

    // 按方案分派编码
    cv::Scalar schemeEncode(Channel::Scheme scheme, int msg)
    {
        if (scheme == Channel::Scheme::Binary)
        {
            return binary_comm::encode(msg);
        }
        return octal_comm::encode(msg);
    }

    // 按方案分派解码
    int schemeDecode(Channel::Scheme scheme, const cv::Scalar& color)
    {
        if (scheme == Channel::Scheme::Binary)
        {
            return binary_comm::decode(color);
        }
        return octal_comm::decode(color);
    }
}

namespace binary_comm
{
    // 二进制编码：黑=1，白=0
    cv::Scalar encode(int msg)
    {
        if (msg != 0 && msg != 1)
        {
            throw std::invalid_argument("binary mode only accepts 0 or 1");
        }

        return msg == 1 ? cv::Scalar(0, 0, 0) : cv::Scalar(255, 255, 255);
    }

    // 二进制解码：在黑白两色中取最近颜色
    int decode(cv::Scalar color)
    {
        const std::array<cv::Scalar, 8> palette = {
            cv::Scalar(255, 255, 255), // 0 -> white
            cv::Scalar(0, 0, 0),       // 1 -> black
        };

        const int index = nearestIndex(color, palette, 2);
        return index == 0 ? 0 : 1;
    }
}

namespace octal_comm
{
    // 八进制编码：0~7 映射到8种固定颜色
    cv::Scalar encode(int msg)
    {
        if (msg < 0 || msg > 7)
        {
            throw std::invalid_argument("octal mode only accepts 0~7");
        }

        static const std::array<cv::Scalar, 8> palette = {
            cv::Scalar(0, 0, 0),       // 0 black
            cv::Scalar(255, 255, 255), // 1 white
            cv::Scalar(0, 0, 255),     // 2 red
            cv::Scalar(255, 0, 0),     // 3 blue
            cv::Scalar(0, 255, 0),     // 4 green
            cv::Scalar(255, 0, 255),   // 5 purple
            cv::Scalar(0, 255, 255),   // 6 yellow
            cv::Scalar(255, 255, 0),   // 7 cyan
        };

        return palette[msg];
    }

    // 八进制解码：在8色调色板中取最近颜色
    int decode(cv::Scalar color)
    {
        static const std::array<cv::Scalar, 8> palette = {
            cv::Scalar(0, 0, 0),
            cv::Scalar(255, 255, 255),
            cv::Scalar(0, 0, 255),
            cv::Scalar(255, 0, 0),
            cv::Scalar(0, 255, 0),
            cv::Scalar(255, 0, 255),
            cv::Scalar(0, 255, 255),
            cv::Scalar(255, 255, 0),
        };

        return nearestIndex(color, palette, 8);
    }
}

Channel::Channel(Scheme scheme)
    : scheme_(scheme)
{
}

void Channel::send(int msg)
{
    // 发送端：全屏显示一个纯色块表示一个符号
    const cv::Scalar color = schemeEncode(scheme_, msg);
    cv::Mat frame(kFrameHeight, kFrameWidth, CV_8UC3, color);

    const std::string modeText = (scheme_ == Scheme::Binary) ? "binary" : "octal";
    const std::string infoText = "mode=" + modeText + " msg=" + std::to_string(msg);
    const cv::Scalar textColor = (msg == 1 && scheme_ == Scheme::Binary) || msg == 0 ? cv::Scalar(255, 255, 255) : cv::Scalar(0, 0, 0);
    cv::putText(frame, infoText, cv::Point(40, 70), cv::FONT_HERSHEY_SIMPLEX, 1.2, textColor, 2, cv::LINE_AA);
    cv::putText(frame, "Press ESC/Enter/Space to close", cv::Point(40, 120), cv::FONT_HERSHEY_SIMPLEX, 0.9, textColor, 2, cv::LINE_AA);

    cv::namedWindow("LightSender", cv::WINDOW_NORMAL);
    cv::setWindowProperty("LightSender", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    const auto start = std::chrono::steady_clock::now();
    constexpr auto kMaxShowTime = std::chrono::seconds(5);
    while (true)
    {
        cv::imshow("LightSender", frame);
        const int key = cv::waitKey(30);
        if (key == 27 || key == 13 || key == 32)
        {
            break;
        }

        const auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= kMaxShowTime)
        {
            break;
        }
    }

    cv::destroyWindow("LightSender");
}

int Channel::receive()
{
    // 接收端：从默认摄像头采样中心区域颜色并解码
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        throw std::runtime_error("failed to open camera");
    }

    cv::Mat frame;
    for (int i = 0; i < 20; ++i)
    {
        cap >> frame;
    }

    if (frame.empty())
    {
        throw std::runtime_error("failed to capture frame");
    }

    const cv::Scalar sampled = sampleCenterColor(frame);
    return schemeDecode(scheme_, sampled);
}
