#ifndef ECSACT_CODEGEN_PLUGIN_H
#define ECSACT_CODEGEN_PLUGIN_H

#include "ecsact/runtime/common.h"

#ifndef ECSACT_CODEGEN_PLUGIN_API
#	ifdef __cplusplus
#		ifdef _WIN32
#			define ECSACT_CODEGEN_PLUGIN_API extern "C" __declspec(dllexport)
#		else
#			define ECSACT_CODEGEN_PLUGIN_API \
				extern "C" __attribute__((visibility("default")))
#		endif
#	else
#		ifdef _WIN32
#			define ECSACT_CODEGEN_PLUGIN_API extern __declspec(dllexport)
#		else
#			define ECSACT_CODEGEN_PLUGIN_API \
				extern __attribute__((visibility("default")))
#		endif
#	endif
#endif // ECSACT_CODEGEN_PLUGIN_API

/**
 * Characters passed to this function are written to file or stdout. Characters
 * that go beyond @p str_len are not read.
 *
 * @NOTE: it is _NOT_ assumed that @p str is null-terminated, you must set
 * @p str_len properly.
 *
 * @param filename_index - index from output filenames from @ref
 * ecsact_codegen_output_filenames. If @ref ecsact_codegen_output_filenames is
 * not defined by the plugin then this parameter must be 0, otherwise it is
 * an error to give a filename index >= to the output filenames length.
 * @param str - array of characters of length @p str_len
 * @param str_len - length of array of characters @p str
 */
typedef void (*ecsact_codegen_write_fn_t)( //
	int32_t     filename_index,
	const char* str,
	int32_t     str_len
);

typedef enum ecsact_codegen_report_type {
	/**
	 * Informational report. Mostly used for debugging. May or may not be shown to
	 * user.
	 */
	ECSACT_CODEGEN_REPORT_INFO,

	/**
	 * Warning. May or may not be shown to user.
	 */
	ECSACT_CODEGEN_REPORT_WARNING,

	/**
	 * An error has occurred, but can still continue running. Will be shown to
	 * user when possible.
	 */
	ECSACT_CODEGEN_REPORT_ERROR,

	/**
	 * An error has occurred and plugin cannot continue running. Will be shown to
	 * user when possible.
	 */
	ECSACT_CODEGEN_REPORT_FATAL,
} ecsact_codegen_report_message_type;

/**
 * Message passed to this function is reported to the codegen host and may be
 * shown to the user. Characters that go beyond @p msg_len are not read. Some
 * report types may hault the codegen process. @see ecsact_codegen_report_type
 *
 * @NOTE: it is _NOT_ assumed that @p msg is null-terminated, you must set
 * @p msg_len properly.
 *
 * @param msg - array of characters of length @p msg_len
 * @param msg_len - length of array of characters @p msg
 */
typedef void (*ecsact_codegen_report_fn_t)( //
	int32_t                    filename_index,
	ecsact_codegen_report_type report_type,
	const char*                msg,
	int32_t                    msg_len
);

/**
 * @param package_id the package
 * @param out_filenames filenames to write to. if `nullptr` this parameter is
 * should be ignored. May only write to a max of @p max_filenames.
 * @param out_filenames_length Must write the length of filenames this plugin
 * writes to
 */
ECSACT_CODEGEN_PLUGIN_API void ecsact_codegen_output_filenames( //
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
);

ECSACT_CODEGEN_PLUGIN_API const char* ecsact_codegen_plugin_name();

/**
 * Ecsact codegen plugin entrypoint. Plugin developers implement this function
 * to write code generation output via the `write_fn` parameter.
 *
 * It is expected that the implementation uses the runtime meta module
 * functions.
 *
 * @param package_id package the codegen plugin writes for
 * @param write_fn implementation calls this function when code generation
 *        output
 */
ECSACT_CODEGEN_PLUGIN_API void ecsact_codegen_plugin( //
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
);

#endif // ECSACT_CODEGEN_PLUGIN_H
