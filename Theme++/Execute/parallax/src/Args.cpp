#include "Args.h"
#include "Constants.h"

#include "../submodules/CLI11/include/CLI/CLI.hpp"

void Args::parse(int argc, char** argv)
{
	CLI::App app(

		APP_LOGO
		"\n\n"

		"Download files over HTTP, in parallel, from servers that support\n"
		"HTTP range requests [1][2]. If range requests are not supported,\n"
		"downloads the file normally.\n"
		"\n"
		"  [1]: https://developer.mozilla.org/docs/Web/HTTP/Range_requests\n"
		"  [2]: https://tools.ietf.org/html/rfc7233\n"

	, "parallax");

	// Positional
	app.add_option("url", url, "URL of the file to download")->required();
	app.add_option("output", output, "Local path to which the file is saved")->required();

	// Options
	app.add_option("-t,--threads", t,"Number of threads to use")->check(CLI::PositiveNumber);

	try {
		app.parse(argc, argv);
	}
	catch (CLI::ParseError &e) {
		std::exit(app.exit(e));
	}
}
