#include <iostream>
#include <clocale>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include <opencv2/imgcodecs.hpp>

#include "light_comm.hpp"

namespace
{
#ifdef _WIN32
    void configureConsoleUtf8()
    {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::setlocale(LC_ALL, ".UTF-8");
    }
#else
    void configureConsoleUtf8()
    {
        std::setlocale(LC_ALL, "C.UTF-8");
    }
#endif

    // 解析调制方案：binary(二进制) 或 octal(八进制)
    Channel::Scheme parseScheme(const std::string& text)
    {
        if (text == "binary")
        {
            return Channel::Scheme::Binary;
        }
        if (text == "octal")
        {
            return Channel::Scheme::Octal;
        }
        throw std::invalid_argument("scheme must be binary or octal");
    }

    // 根据方案调用对应编码接口
    cv::Scalar encodeByScheme(Channel::Scheme scheme, int msg)
    {
        return scheme == Channel::Scheme::Binary ? binary_comm::encode(msg) : octal_comm::encode(msg);
    }

    // 根据方案调用对应解码接口
    int decodeByScheme(Channel::Scheme scheme, const cv::Scalar& color)
    {
        return scheme == Channel::Scheme::Binary ? binary_comm::decode(color) : octal_comm::decode(color);
    }

    void printUsage()
    {
        std::cout
            << "用法:\n"
            << "  LightCorrespondence send <binary|octal> <msg>\n"
            << "  LightCorrespondence receive <binary|octal>\n"
            << "  LightCorrespondence simulate <binary|octal> <msg> <image_path>\n"
            << "  LightCorrespondence decode-image <binary|octal> <image_path>\n";
    }

    void printRunTips()
    {
        std::cout
            << "\n运行提示:\n"
            << "  1) send: 需要输入要传输的数字\n"
            << "     - binary 模式只能传 0 或 1\n"
            << "     - octal  模式只能传 0~7\n"
            << "  2) receive: 直接调用摄像头接收一个符号\n"
            << "  3) simulate/decode-image: 用图像文件离线验证\n"
            << "\n示例:\n"
            << "  LightCorrespondence send binary 1\n"
            << "  LightCorrespondence receive octal\n"
            << "  LightCorrespondence simulate octal 3 frame.png\n"
            << "  LightCorrespondence decode-image octal frame.png\n\n";
    }
}

int main(int argc, char* argv[])
{
    configureConsoleUtf8();

    try
    {
        std::string command;
        std::string schemeText;
        std::string msgText;
        std::string imagePath;

        if (argc == 1)
        {
            std::cout << "未检测到命令行参数，进入交互模式。\n";
            printRunTips();

            std::cout << "请选择运行方式(send/receive/simulate/decode-image): ";
            std::cin >> command;
            std::cout << "请选择传输方案(binary/octal): ";
            std::cin >> schemeText;

            if (command == "send" || command == "simulate")
            {
                std::cout << "请输入要传输的数字: ";
                std::cin >> msgText;
            }

            if (command == "simulate" || command == "decode-image")
            {
                std::cout << "请输入图像路径: ";
                std::cin >> imagePath;
            }
        }
        else
        {
            if (argc < 3)
            {
                printUsage();
                printRunTips();
                return 1;
            }

            command = argv[1];
            schemeText = argv[2];
            if (argc >= 4)
            {
                msgText = argv[3];
            }
            if (argc >= 5)
            {
                imagePath = argv[4];
            }
        }

        const Channel::Scheme scheme = parseScheme(schemeText);

        if (command == "send")
        {
            if (msgText.empty())
            {
                printUsage();
                printRunTips();
                return 1;
            }

            // 发送一个符号（binary: 0/1, octal: 0~7）
            const int msg = std::stoi(msgText);
            Channel channel(scheme);
            channel.send(msg);
            return 0;
        }

        if (command == "receive")
        {
            // 接收一个符号并输出到终端
            Channel channel(scheme);
            const int msg = channel.receive();
            std::cout << msg << std::endl;
            return 0;
        }

        if (command == "simulate")
        {
            if (msgText.empty() || imagePath.empty())
            {
                printUsage();
                printRunTips();
                return 1;
            }

            // 生成纯色图像，便于离线验证解码逻辑
            const int msg = std::stoi(msgText);
            const cv::Scalar color = encodeByScheme(scheme, msg);
            const cv::Mat image(720, 1280, CV_8UC3, color);
            cv::imwrite(imagePath, image);
            std::cout << "已保存: " << imagePath << std::endl;
            return 0;
        }

        if (command == "decode-image")
        {
            if (imagePath.empty())
            {
                printUsage();
                printRunTips();
                return 1;
            }

            const cv::Mat image = cv::imread(imagePath);
            if (image.empty())
            {
                throw std::runtime_error("读取图像失败: " + imagePath);
            }

            // 对整张图取均值颜色，再解码为符号
            const cv::Scalar avg = cv::mean(image);
            std::cout << decodeByScheme(scheme, avg) << std::endl;
            return 0;
        }

        printUsage();
        printRunTips();
        return 1;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "错误: " << ex.what() << std::endl;
        return 2;
    }
}
