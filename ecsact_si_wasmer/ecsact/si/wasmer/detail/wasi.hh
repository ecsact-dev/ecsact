#ifndef ECSACT_SI_WASI_H
#define ECSACT_SI_WASI_H

#include <wasm.h>
#include <cstdint>

/**
 * A region of memory for scatter/gather writes.
 */
typedef struct ecsact_si_wasi_ciovec_t {
	/**
	 * The guest pointer of the buffer to be written.
	 */
	int32_t buf;

	/**
	 * The length of the buffer to be written.
	 */
	int32_t buf_len;
} ecsact_si_wasi_ciovec_t;

static_assert(sizeof(ecsact_si_wasi_ciovec_t) == 8);
static_assert(alignof(ecsact_si_wasi_ciovec_t) == 4);
// static_assert(offsetof(ecsact_si_wasi_ciovec_t, buf) == 0);
// static_assert(offsetof(ecsact_si_wasi_ciovec_t, buf_len) == 4);

enum class ecsact_si_wasi_filetype : uint8_t {
	/**
	 * The type of the file descriptor or file is unknown or is different from any
	 * of the other types specified.
	 */
	unknown = 0,

	/**
	 * The file descriptor or file refers to a block device inode.
	 */
	block_device = 1,

	/**
	 * The file descriptor or file refers to a character device inode.
	 */
	character_device = 2,

	/**
	 * The file descriptor or file refers to a directory inode.
	 */
	directory = 3,

	/**
	 * The file descriptor or file refers to a regular file inode.
	 */
	regular_file = 4,

	/**
	 * The file descriptor or file refers to a datagram socket.
	 */
	socket_dgram = 5,

	/**
	 * The file descriptor or file refers to a byte-stream socket.
	 */
	socket_stream = 6,

	/**
	 * The file refers to a symbolic link inode.
	 */
	symbolic_link = 7,
};

enum class ecsact_si_wasi_fdflags : uint16_t {
	/**
	 * Append mode: Data written to the file is always appended to the file's end.
	 */
	append = 1 << 0,

	/**
	 * Write according to synchronized I/O data integrity completion. Only the
	 * data stored in the file is synchronized.
	 */
	dsync = 1 << 1,

	/**
	 * Non-blocking mode.
	 */
	nonblock = 1 << 2,

	/**
	 * Synchronized read I/O operations.
	 */
	rsync = 1 << 3,

	/**
	 * Write according to synchronized I/O file integrity completion. In
	 * addition to synchronizing the data stored in the file, the implementation
	 * may also synchronously update the file's metadata.
	 */
	sync = 1 << 4,
};

inline ecsact_si_wasi_fdflags operator|(
	ecsact_si_wasi_fdflags a,
	ecsact_si_wasi_fdflags b
) {
	return static_cast<ecsact_si_wasi_fdflags>(
		static_cast<uint16_t>(a) | static_cast<uint16_t>(b)
	);
}

inline ecsact_si_wasi_fdflags operator&(
	ecsact_si_wasi_fdflags a,
	ecsact_si_wasi_fdflags b
) {
	return static_cast<ecsact_si_wasi_fdflags>(
		static_cast<uint16_t>(a) & static_cast<uint16_t>(b)
	);
}

/**
 * File descriptor rights, determining which actions may be performed.
 */
enum class ecsact_si_wasi_rights : uint64_t {
	/**
	 * The right to invoke `fd_datasync`.
	 * If `path_open` is set, includes the right to invoke
	 * `path_open` with `fdflags::dsync`.
	 */
	fd_datasync = 1 << 0,

	/**
	 * The right to invoke `fd_read` and `sock_recv`.
	 * If `rights::fd_seek` is set, includes the right to invoke `fd_pread`.
	 */
	fd_read = 1 << 1,

	/**
	 * The right to invoke `fd_seek`. This flag implies `rights::fd_tell`.
	 */
	fd_seek = 1 << 2,

	/**
	 * The right to invoke `fd_fdstat_set_flags`.
	 */
	fd_fdstat_set_flags = 1 << 3,

	/**
	 * The right to invoke `fd_sync`.
	 * If `path_open` is set, includes the right to invoke
	 * `path_open` with `fdflags::rsync` and `fdflags::dsync`.
	 */
	fd_sync = 1 << 4,

	/**
	 * The right to invoke `fd_seek` in such a way that the file offset
	 * remains unaltered (i.e., `whence::cur` with offset zero), or to
	 * invoke `fd_tell`.
	 */
	fd_tell = 1 << 5,

	/**
	 * The right to invoke `fd_write` and `sock_send`.
	 * If `rights::fd_seek` is set, includes the right to invoke `fd_pwrite`.
	 */
	fd_write = 1 << 6,

	/**
	 * The right to invoke `fd_advise`.
	 */
	fd_advise = 1 << 7,

	/**
	 * The right to invoke `fd_allocate`.
	 */
	fd_allocate = 1 << 8,

	/**
	 * The right to invoke `path_create_directory`.
	 */
	path_create_directory = 1 << 9,

	/**
	 * If `path_open` is set, the right to invoke `path_open` with
	 * `oflags::creat`.
	 */
	path_create_file = 1 << 10,

	/**
	 * The right to invoke `path_link` with the file descriptor as the
	 * source directory.
	 */
	path_link_source = 1 << 11,

	/**
	 * The right to invoke `path_link` with the file descriptor as the
	 * target directory.
	 */
	path_link_target = 1 << 12,

	/**
	 * The right to invoke `path_open`.
	 */
	path_open = 1 << 13,

	/**
	 * The right to invoke `fd_readdir`.
	 */
	fd_readdir = 1 << 14,

	/**
	 * The right to invoke `path_readlink`.
	 */
	path_readlink = 1 << 15,

	/**
	 * The right to invoke `path_rename` with the file descriptor as the source
	 * directory.
	 */
	path_rename_source = 1 << 16,

	/**
	 * The right to invoke `path_rename` with the file descriptor as the target
	 * directory.
	 */
	path_rename_target = 1 << 17,

	/**
	 * The right to invoke `path_filestat_get`.
	 */
	path_filestat_get = 1 << 18,

	/**
	 * The right to change a file's size (there is no `path_filestat_set_size`).
	 * If `path_open` is set, includes the right to invoke `path_open` with
	 * `oflags::trunc`.
	 */
	path_filestat_set_size = 1 << 19,

	/**
	 * The right to invoke `path_filestat_set_times`.
	 */
	path_filestat_set_times = 1 << 20,

	/**
	 * The right to invoke `fd_filestat_get`.
	 */
	fd_filestat_get = 1 << 21,

	/**
	 * The right to invoke `fd_filestat_set_size`.
	 */
	fd_filestat_set_size = 1 << 22,

	/**
	 * The right to invoke `fd_filestat_set_times`.
	 */
	fd_filestat_set_times = 1 << 23,

	/**
	 * The right to invoke `path_symlink`.
	 */
	path_symlink = 1 << 24,

	/**
	 * The right to invoke `path_remove_directory`.
	 */
	path_remove_directory = 1 << 25,

	/**
	 * The right to invoke `path_unlink_file`.
	 */
	path_unlink_file = 1 << 26,

	/**
	 * If `rights::fd_read` is set, includes the right to invoke `poll_oneoff` to
	 * subscribe to `eventtype::fd_read`. If `rights::fd_write` is set, includes
	 * the right to invoke `poll_oneoff` to subscribe to `eventtype::fd_write`.
	 */
	poll_fd_readwrite = 1 << 27,

	/**
	 * The right to invoke `sock_shutdown`.
	 */
	sock_shutdown = 1 << 28,

	/**
	 * The right to invoke `sock_accept`.
	 */
	sock_accept = 1 << 29,
};

inline ecsact_si_wasi_rights operator|(
	ecsact_si_wasi_rights a,
	ecsact_si_wasi_rights b
) {
	return static_cast<ecsact_si_wasi_rights>(
		static_cast<uint64_t>(a) | static_cast<uint64_t>(b)
	);
}

inline ecsact_si_wasi_rights operator&(
	ecsact_si_wasi_rights a,
	ecsact_si_wasi_rights b
) {
	return static_cast<ecsact_si_wasi_rights>(
		static_cast<uint64_t>(a) & static_cast<uint64_t>(b)
	);
}

typedef struct ecsact_si_wasi_fdstat_t {
	/**
	 * File type.
	 */
	ecsact_si_wasi_filetype fs_filetype;

	/**
	 * File descriptor flags.
	 */
	ecsact_si_wasi_fdflags fs_flags;

	/**
	 * Rights that apply to this file descriptor.
	 */
	ecsact_si_wasi_rights fs_rights_base;

	/**
	 * Maximum set of rights that may be installed on new file descriptors that
	 * are created through this file descriptor, e.g., through `path_open`.
	 */
	ecsact_si_wasi_rights fs_rights_inheriting;

} ecsact_si_wasi_fdstat_t;

static_assert(sizeof(ecsact_si_wasi_fdstat_t) == 24);

/**
 * Ecsact system implementation exited prematurely. Unlike normal usage of this
 * function @p exit_code being `0` does NOT mean success.
 * @param exit_code may be logged to the host, but otherwise is meaningless to
 *        the ecsact wasm host.
 */
// void ecsact_si_wasi_proc_exit(int32_t exit_code);
wasm_trap_t* ecsact_si_wasi_proc_exit(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_fd_seek(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_fd_write(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_fd_read(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_fd_close(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_environ_sizes_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_environ_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

wasm_trap_t* ecsact_si_wasi_fd_fdstat_get(
	const wasm_val_vec_t* args,
	wasm_val_vec_t*       results
);

#endif // ECSACT_SI_WASI_H
