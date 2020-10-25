#include <iostream>
#include <mutex>

#include "Args.h"
#include "Constants.h"
#include "../../libparallax/src/Parallax.h"

struct ProgressBar
{
	int width;

	const char* filledChar = "█";
	const char* emptyChar = "░";

	int intProgress = -1;
	std::mutex m {};

	void print(double newProgress)
	{
		if ((newProgress * 100) > intProgress)
		{
			m.lock();
			intProgress = (int)(newProgress * 100);
			int filled = (int)(newProgress * width);

			std::cout << "\r|";
			for (int i = 0; i < width; i++)
			{
				std::cout << (i < filled ? filledChar : emptyChar);
			}
			std::cout << "| " << intProgress << "%";
			std::cout.flush();

			m.unlock();
		}
	}
};

int main(int argc, char** argv)
{
	// Parse args.
	Args args;
	args.parse(argc, argv);

	std::cout << APP_LOGO << "\n\n";
	std::cout << "Downloading: " << args.url << '\n';
	std::cout << "Output:      " << args.output << '\n';
	std::cout << "Threads:     " << args.t << '\n';
	std::cout << "\nProgress:" << std::endl;

	ProgressBar p { 60 };
	p.print(0);

	// Download.
	try
	{
		Parallax::ParallaxResult result = Parallax::download(args.url, args.output, args.t, [&p](double progress)
		{
			p.print(progress);
		});

		auto ns = result.duration.count();
		std::cout << "\n\n";

		std::cout << "Size:          " << result.contentLength << " bytes" << '\n';

		if (!result.contentType.empty())
			std::cout << "Content type:  " << result.contentType << '\n';

		std::cout << "Downloaded in: " << ns << "ns (" << ns / 1e9f << "s)" << std::endl;
		return 0;
	}
	catch (Parallax::ParallaxException& e)
	{
		std::cerr << "\n\nError: " << e.what() << std::endl;
		return -1;
	}
}
