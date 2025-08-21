#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>

std::string escape_cpp_value(std::string value) {
	if(value == "true" || value == "false") {
		return value;
	}

	return "\"" + value + "\"";
}

void write_cc_constexpr_lines(
	std::ifstream& status_file_stream,
	std::ofstream& output_stream
) {
	std::string            line;
	std::string::size_type ws_idx;

	// Reading file based on:
	// https://docs.bazel.build/versions/master/user-manual.html#workspace_status
	while(status_file_stream.good()) {
		line.clear();
		std::getline(status_file_stream, line);
		if(line.empty()) {
			continue;
		}

		ws_idx = line.find_first_of(' ');

		output_stream << "constexpr auto " << line.substr(0, ws_idx) // key
									<< " = " << escape_cpp_value(line.substr(ws_idx + 1))
									<< ";\n"; // value
	}
}

int main(int argc, char* argv[]) {
	std::string stable_status_path;
	std::string volatile_status_path;
	std::string output_header_path;

	for(int i = 1; argc > i; ++i) {
		std::string arg(argv[i]);

		if(arg == "--stable_status") {
			stable_status_path = std::string(argv[++i]);
		} else if(arg == "--volatile_status") {
			volatile_status_path = std::string(argv[++i]);
		} else if(arg == "--output_header") {
			output_header_path = std::string(argv[++i]);
		} else {
			std::cerr << "invalid argument: " << arg << std::endl;
			return 1;
		}
	}

	if(stable_status_path.empty()) {
		std::cerr << "missing --stable_status" << std::endl;
		return 1;
	}

	if(volatile_status_path.empty()) {
		std::cerr << "missing --volatile_status" << std::endl;
		return 1;
	}

	if(output_header_path.empty()) {
		std::cerr << "missing --output_header" << std::endl;
		return 1;
	}

	try {
		std::ifstream stable_status_stream(stable_status_path);
		std::ifstream volatile_status_stream(volatile_status_path);
		std::ofstream output_header_stream(output_header_path);

		output_header_stream << "#pragma once\n\n"
												 << "// THIS FILE IS GENERATED - DO NOT EDIT\n\n";

		write_cc_constexpr_lines(stable_status_stream, output_header_stream);
		write_cc_constexpr_lines(volatile_status_stream, output_header_stream);
	} catch(const std::exception& err) {
		std::cerr << "EXCEPTION THROWN: err.what() == " << err.what() << std::endl;
		return 2;
	}

	return 0;
}
