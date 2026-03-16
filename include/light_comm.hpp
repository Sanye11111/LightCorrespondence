#pragma once

#include <opencv2/core.hpp>

namespace binary_comm
{
    cv::Scalar encode(int msg);
    int decode(cv::Scalar color);
}

namespace octal_comm
{
    cv::Scalar encode(int msg);
    int decode(cv::Scalar color);
}

class Channel
{
public:
    enum class Scheme
    {
        Binary,
        Octal
    };

    explicit Channel(Scheme scheme);

    void send(int msg);
    int receive();

private:
    Scheme scheme_;
};
